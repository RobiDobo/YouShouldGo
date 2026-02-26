/**
 * App Module - Orchestrates the entire application
 * Handles event listeners and initialization
 */

// Tracking intervals
let trackingInterval = null;
let stationListInterval = null;

/**
 * Load and display available agencies
 */
async function loadAgencies() {
    try {
        const agencies = await API.getAgencies();
        populateAgencies(agencies);
        
        // Auto-select Cluj (only available agency)
        if (agencies.length === 1) {
            AppState.selectedAgencyId = agencies[0].agency_id;
            DOM.agencySelect.value = AppState.selectedAgencyId;
            DOM.agencySelect.dispatchEvent(new Event('change'));
        }
    } catch (error) {
        showStatus('Error loading agencies: ' + error.message, 'error');
    }
}

/**
 * Agency selection handler
 */
DOM.agencySelect.addEventListener('change', async () => {
    AppState.selectedAgencyId = DOM.agencySelect.value;
    DOM.routeSelect.disabled = true;
    DOM.tripSelect.disabled = true;
    DOM.startBtn.disabled = true;
    
    if (!AppState.selectedAgencyId) {
        resetRoutesDropdown();
        resetTripsDropdown();
        hideStatus();
        return;
    }
    
    try {
        showStatus('Loading routes with active vehicles...', 'info');
        const routes = await API.getRoutesWithVehicles();
        populateRoutes(routes);
        
        if (routes.length === 0) {
            showStatus('No routes with active vehicles found.', 'info');
            return;
        }
        
        DOM.routeSelect.disabled = false;
        resetTripsDropdown();
        hideStatus();
    } catch (error) {
        showStatus('Error loading routes: ' + error.message, 'error');
        console.error('Error:', error);
    }
});

/**
 * Route selection handler
 */
DOM.routeSelect.addEventListener('change', async () => {
    AppState.selectedRouteId = DOM.routeSelect.value;
    DOM.tripSelect.disabled = true;
    DOM.startBtn.disabled = true;
    
    if (!AppState.selectedRouteId) {
        resetTripsDropdown();
        hideStatus();
        return;
    }
    
    try {
        showStatus('Loading directions...', 'info');
        const trips = await API.getTripsForRoute(AppState.selectedRouteId);

        AppState.tripsById = {};
        trips.forEach(trip => { AppState.tripsById[trip.trip_id] = trip; });
        populateTrips(trips);
        
        if (trips.length === 0) {
            showStatus('No trips found for this route.', 'info');
            return;
        }
        
        DOM.tripSelect.disabled = false;
        hideStatus();
        hideDirectionInfo();
    } catch (error) {
        showStatus('Error loading trips: ' + error.message, 'error');
        console.error('Error:', error);
    }
});

/**
 * Trip selection handler
 */
DOM.tripSelect.addEventListener('change', () => {
    AppState.selectedTripId = DOM.tripSelect.value;
    AppState.selectedTripHeadsign = AppState.selectedTripId ? AppState.tripsById[AppState.selectedTripId]?.trip_headsign : null;
    
    if (AppState.selectedTripHeadsign) {
        showDirectionInfo(`Direction: ${AppState.selectedTripHeadsign}`);
    } else {
        hideDirectionInfo();
    }
    
    DOM.startBtn.disabled = !AppState.selectedTripId;

    if (AppState.selectedTripId) {
        DOM.mapDiv.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
});

/**
 * Start tracking button handler
 */
DOM.startBtn.addEventListener('click', async () => {
    if (!AppState.selectedTripId) return;
    
    try {
        showStatus('Starting tracking...', 'info');
        
        // Set selected trip in backend
        await API.selectTrip(AppState.selectedTripId);
        
        showStatus('Tracking started! Vehicles will update every 15 seconds.', 'success');
        
        // Clear old markers from map
        clearMap();

        // Load stations for the selected trip
        await loadStations();
        
        // Initial update
        updateTrams();
        updateStationList();
        
        // Set up periodic updates
        if (trackingInterval) clearInterval(trackingInterval);
        if (stationListInterval) clearInterval(stationListInterval);
        
        trackingInterval = setInterval(updateTrams, CONFIG.UPDATE_INTERVAL);
        stationListInterval = setInterval(updateStationList, CONFIG.UPDATE_INTERVAL);
        
    } catch (error) {
        showStatus('Error starting tracking: ' + error.message, 'error');
        console.error('Error:', error);
    }
});

/**
 * Initialize application on page load
 */
function initializeApp() {
    loadAgencies();
    updateStatusMessage();
    setInterval(updateStatusMessage, CONFIG.UPDATE_INTERVAL);
}

// Start the app when page is ready
initializeApp();
