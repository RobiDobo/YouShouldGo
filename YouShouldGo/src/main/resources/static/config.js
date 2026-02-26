
    // State
    const AppState = {
    selectedAgencyId: null,
    selectedRouteId: null,
    selectedTripId: null,
    selectedTripHeadsign: null,
    tripsById: {}
    }
    // DOM elements
    const DOM = {
        agencySelect: document.getElementById('agencySelect'),
        routeSelect: document.getElementById('routeSelect'),
        tripSelect: document.getElementById('tripSelect'),
        startBtn: document.getElementById('startTracking'),
        statusDiv: document.getElementById('status'),
        directionInfo: document.getElementById('directionInfo'),
        statusMessage: document.getElementById('statusMessage'),
        mapDiv: document.getElementById('map')
    };

// Constants
const CONFIG = {
    UPDATE_INTERVAL: 15000,
    MAP_CENTER: [46.7712, 23.6236],
    MAP_ZOOM: 13
};