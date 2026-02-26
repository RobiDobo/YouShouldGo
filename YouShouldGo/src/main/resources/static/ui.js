/**
 * UI Module - Handles all DOM updates and user interface rendering
 * Manages status messages, dropdowns, and station list display
 */

/**
 * Show status message to user
 */
function showStatus(message, type = 'info') {
    DOM.statusDiv.textContent = message;
    DOM.statusDiv.className = `status ${type}`;
    DOM.statusDiv.style.display = 'block';
}

/**
 * Hide status message
 */
function hideStatus() {
    DOM.statusDiv.style.display = 'none';
}

/**
 * Show direction/route information banner
 */
function showDirectionInfo(message) {
    DOM.directionInfo.textContent = message;
    DOM.directionInfo.style.display = 'block';
}

/**
 * Hide direction information banner
 */
function hideDirectionInfo() {
    DOM.directionInfo.style.display = 'none';
}

/**
 * Update status message from API (sent to microcontroller)
 */
async function updateStatusMessage() {
    try {
        const text = await API.getStatus();
        DOM.statusMessage.textContent = text;
    } catch (error) {
        DOM.statusMessage.textContent = 'Status unavailable';
        console.error('Error:', error);
    }
}

/**
 * Populate agencies dropdown
 */
function populateAgencies(agencies) {
    DOM.agencySelect.innerHTML = '<option value="">-- Select an agency --</option>';
    agencies.forEach(agency => {
        const option = document.createElement('option');
        option.value = agency.agency_id;
        option.textContent = agency.agency_name;
        DOM.agencySelect.appendChild(option);
    });
}

/**
 * Populate routes dropdown
 */
function populateRoutes(routes) {
    if (routes.length === 0) {
        DOM.routeSelect.innerHTML = '<option value="">No active routes available</option>';
        return;
    }
    
    DOM.routeSelect.innerHTML = '<option value="">-- Select a route --</option>';
    routes.forEach(route => {
        const option = document.createElement('option');
        option.value = route.route_id;
        option.textContent = `${route.route_short_name} - ${route.route_long_name}`;
        DOM.routeSelect.appendChild(option);
    });
}

/**
 * Populate trips/directions dropdown
 */
function populateTrips(trips) {
    if (trips.length === 0) {
        DOM.tripSelect.innerHTML = '<option value="">No trips available</option>';
        return;
    }
    
    DOM.tripSelect.innerHTML = '<option value="">-- Select a direction --</option>';
    trips.forEach(trip => {
        const option = document.createElement('option');
        option.value = trip.trip_id;
        option.textContent = trip.trip_headsign;
        DOM.tripSelect.appendChild(option);
    });
}

/**
 * Reset routes dropdown to initial state
 */
function resetRoutesDropdown() {
    DOM.routeSelect.innerHTML = '<option value="">First select an agency</option>';
    DOM.routeSelect.disabled = true;
}

/**
 * Reset trips dropdown to initial state
 */
function resetTripsDropdown() {
    DOM.tripSelect.innerHTML = '<option value="">First select a route</option>';
    DOM.tripSelect.disabled = true;
}

/**
 * Update station list with vehicle information
 */
async function updateStationList() {
    try {
        const stations = await API.getStationsWithVehicles();
        
        const stationList = document.getElementById('stationListItems');
        
        if (stations.length === 0) {
            stationList.innerHTML = '<li style="text-align: center; color: #999; padding: 20px;">No stations available</li>';
            return;
        }
        
        stationList.innerHTML = stations.map(station => {
            const hasVehicleClass = station.hasVehicle === 1 ? 'has-vehicle' : '';
            const vehicleInfo = station.vehicles.map(v => 
                `${v.label} (${v.speed?.toFixed(1) || '0.0'} m/s)`
            ).join('<br>');
            
            return `
                <li class="station-item ${hasVehicleClass}" 
                    data-sequence="${station.sequence}"
                    data-lat="${station.lat}"
                    data-lon="${station.lon}"
                    data-name="${station.stationName}"
                    style="cursor: pointer;">
                    <span class="sequence">${station.sequence}.</span> ${station.stationName}
                    ${vehicleInfo ? `<div class="vehicle-info">${vehicleInfo}</div>` : ''}
                </li>
            `;
        }).join('');
        
        // Add click handlers to station items
        document.querySelectorAll('.station-item').forEach(item => {
            item.addEventListener('click', async () => {
                const lat = parseFloat(item.dataset.lat);
                const lon = parseFloat(item.dataset.lon);
                const name = item.dataset.name;
                const sequence = parseInt(item.dataset.sequence);
                await selectStation(lat, lon, name, sequence);
            });
        });
        
    } catch (error) {
        console.error('Error updating station list:', error);
        document.getElementById('stationListItems').innerHTML = 
            '<li style="text-align: center; color: #dc3545; padding: 20px;">Error loading stations</li>';
    }
}
