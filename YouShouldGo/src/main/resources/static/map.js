/**
 * Map Module - Handles all Leaflet map operations
 * Manages markers, routes, vehicles, and station interactions
 */

// Initialize map
const map = L.map('map').setView(CONFIG.MAP_CENTER, CONFIG.MAP_ZOOM);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);

// Map state
const MapState = {
    markers: {},           // tram markers
    stationMarkers: {},    // station markers
    selectedStationMarker: null,
    routeLine: null,
    routeArrow: null
};

/**
 * Calculate bearing between two coordinates
 * Used for drawing directional arrow on route
 */
function getBearing(lat1, lon1, lat2, lon2) {
    const toRad = (deg) => deg * Math.PI / 180;
    const toDeg = (rad) => rad * 180 / Math.PI;
    const dLon = toRad(lon2 - lon1);
    const y = Math.sin(dLon) * Math.cos(toRad(lat2));
    const x = Math.cos(toRad(lat1)) * Math.sin(toRad(lat2)) -
              Math.sin(toRad(lat1)) * Math.cos(toRad(lat2)) * Math.cos(dLon);
    const brng = toDeg(Math.atan2(y, x));
    return (brng + 360) % 360;
}

/**
 * Clear all map markers and routes
 */
function clearMap() {
    Object.values(MapState.markers).forEach(marker => map.removeLayer(marker));
    MapState.markers = {};
    
    Object.values(MapState.stationMarkers).forEach(marker => map.removeLayer(marker));
    MapState.stationMarkers = {};
    
    MapState.selectedStationMarker = null;
    
    if (MapState.routeLine) {
        map.removeLayer(MapState.routeLine);
        MapState.routeLine = null;
    }
    
    if (MapState.routeArrow) {
        map.removeLayer(MapState.routeArrow);
        MapState.routeArrow = null;
    }
}

/**
 * Load stations and add clickable markers to map
 */
async function loadStations() {
    try {
        const mapData = await API.getStationMap();
        const stops = Object.values(mapData);
        const stopsSorted = stops.slice().sort((a, b) => a.sequence - b.sequence);

        if (stops.length === 0) {
            showStatus('No stations available for this trip.', 'info');
            return;
        }

        stopsSorted.forEach(stop => {
            const marker = L.circleMarker([stop.lat, stop.lon], {
                radius: 6,
                color: '#0c5460',
                fillColor: '#17a2b8',
                fillOpacity: 0.8
            })
            .addTo(map)
            .bindPopup(`<b>${stop.name}</b>`);

            marker.on('click', async () => {
                try {
                    await API.setUserLocation(stop.lat, stop.lon, stop.name);

                    if (MapState.selectedStationMarker) {
                        MapState.selectedStationMarker.setStyle({ color: '#0c5460', fillColor: '#17a2b8' });
                    }
                    marker.setStyle({ color: '#155724', fillColor: '#28a745' });
                    MapState.selectedStationMarker = marker;

                    // Highlight station in list
                    document.querySelectorAll('.station-item').forEach(item => {
                        item.classList.remove('selected');
                    });
                    const listItem = document.querySelector(`[data-sequence="${stop.sequence}"]`);
                    if (listItem) {
                        listItem.classList.add('selected');
                        listItem.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                    }

                    showStatus(`Location set to: ${stop.name}`, 'success');
                    updateStatusMessage();
                } catch (error) {
                    showStatus('Error setting location: ' + error.message, 'error');
                    console.error('Error:', error);
                }
            });

            MapState.stationMarkers[stop.name] = marker;
        });

        // Draw route line connecting stations
        const lineLatLngs = stopsSorted.map(stop => [stop.lat, stop.lon]);
        MapState.routeLine = L.polyline(lineLatLngs, { color: '#17a2b8', weight: 3, opacity: 0.7 }).addTo(map);

        // Add a directional arrow at the last stop
        if (stopsSorted.length >= 2) {
            const prev = stopsSorted[stopsSorted.length - 2];
            const last = stopsSorted[stopsSorted.length - 1];
            const bearing = getBearing(prev.lat, prev.lon, last.lat, last.lon);
            const arrowRotation = bearing - 90;
            const arrowIcon = L.divIcon({
                className: 'route-arrow',
                html: '<span style="display:inline-block; transform: rotate(' + arrowRotation + 'deg);">></span>'
            });
            MapState.routeArrow = L.marker([last.lat, last.lon], { icon: arrowIcon, interactive: false }).addTo(map);
        }

        const lastStop = stopsSorted.reduce((maxStop, stop) => {
            return maxStop == null || stop.sequence > maxStop.sequence ? stop : maxStop;
        }, null);
        
        if (lastStop && AppState.selectedTripHeadsign) {
            showDirectionInfo(`Direction: ${AppState.selectedTripHeadsign} (last stop: ${lastStop.name})`);
        } else if (lastStop) {
            showDirectionInfo(`Last stop on route: ${lastStop.name}`);
        }
    } catch (error) {
        showStatus('Error loading stations: ' + error.message, 'error');
        console.error('Error:', error);
    }
}

/**
 * Update vehicle positions on map
 */
async function updateTrams() {
    try {
        const trams = await API.getVehicles();
        
        if (trams.length === 0) {
            showStatus('No vehicles currently active for this trip.', 'info');
            return;
        }
        
        trams.forEach(tram => {
            if (MapState.markers[tram.id]) {
                // Move existing marker
                MapState.markers[tram.id].setLatLng([tram.latitude, tram.longitude]);
            } else {
                // Create new marker
                MapState.markers[tram.id] = L.marker([tram.latitude, tram.longitude])
                    .addTo(map)
                    .bindPopup(`<b>Vehicle: ${tram.label}</b><br>Speed: ${tram.speed} m/s`);
            }
        });
        
    } catch (error) {
        console.error('Error updating trams:', error);
    }
}

/**
 * Select a station (highlight and update)
 * Called from station list
 */
async function selectStation(lat, lon, name, sequence) {
    try {
        await API.setUserLocation(lat, lon, name);
        
        // Update map marker
        const stationMarker = Object.values(MapState.stationMarkers).find(m => 
            Math.abs(m.getLatLng().lat - lat) < 0.0001 && 
            Math.abs(m.getLatLng().lng - lon) < 0.0001
        );
        
        if (MapState.selectedStationMarker) {
            MapState.selectedStationMarker.setStyle({ color: '#0c5460', fillColor: '#17a2b8' });
        }
        if (stationMarker) {
            stationMarker.setStyle({ color: '#155724', fillColor: '#28a745' });
            MapState.selectedStationMarker = stationMarker;
            map.setView([lat, lon], 15);
        }
        
        // Highlight in list
        document.querySelectorAll('.station-item').forEach(i => i.classList.remove('selected'));
        const listItem = document.querySelector(`[data-sequence="${sequence}"]`);
        if (listItem) {
            listItem.classList.add('selected');
        }
        
        showStatus(`Location set to: ${name}`, 'success');
        updateStatusMessage();
    } catch (error) {
        showStatus('Error setting location: ' + error.message, 'error');
        console.error('Error:', error);
    }
}
