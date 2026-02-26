/**
 * API Module - Handles all backend communication
 * All fetch calls to the Spring Boot backend are centralized here
 */

const API = {
    /**
     * Get all available transit agencies
     * @returns {Promise<Array>} List of agencies
     */
    async getAgencies() {
        const response = await fetch('/api/agencies');
        if (!response.ok) {
            throw new Error(`Failed to load agencies: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Get all routes for the selected agency
     * @returns {Promise<Array>} List of routes
     */
    async getRoutes() {
        const response = await fetch('/api/routes');
        if (!response.ok) {
            throw new Error(`Failed to load routes: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Get only routes that currently have active vehicles
     * @returns {Promise<Array>} List of routes with vehicles
     */
    async getRoutesWithVehicles() {
        const response = await fetch('/api/routes-with-vehicles');
        if (!response.ok) {
            throw new Error(`Failed to load routes: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Get all trips for a specific route
     * @param {number} routeId - The route ID
     * @returns {Promise<Array>} List of trips
     */
    async getTripsForRoute(routeId) {
        const response = await fetch(`/api/trips?routeId=${routeId}`);
        if (!response.ok) {
            throw new Error(`Failed to load trips: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Select a trip for tracking (sets backend state)
     * @param {string} tripId - The trip ID to track
     * @returns {Promise<Response>} Response object
     */
    async selectTrip(tripId) {
        const response = await fetch(`/api/trips/select?tripId=${tripId}`, {
            method: 'POST'
        });
        if (!response.ok) {
            throw new Error(`Failed to select trip: ${response.statusText}`);
        }
        return response;
    },

    /**
     * Get station/stop map for the selected trip
     * @returns {Promise<Object>} Map of stop IDs to stop data
     */
    async getStationMap() {
        const response = await fetch('/api/map');
        if (!response.ok) {
            throw new Error(`Failed to load stations: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Get current vehicle positions for the selected trip
     * @returns {Promise<Array>} List of vehicles with positions
     */
    async getVehicles() {
        const response = await fetch('/api/vehicles');
        if (!response.ok) {
            throw new Error(`Failed to load vehicles: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Get stations with vehicle information (combined endpoint)
     * @returns {Promise<Array>} List of stations with vehicle data
     */
    async getStationsWithVehicles() {
        const response = await fetch('/api/stations-with-vehicles');
        if (!response.ok) {
            throw new Error(`Failed to load stations: ${response.statusText}`);
        }
        return await response.json();
    },

    /**
     * Set user's location (selected station)
     * @param {number} lat - Latitude
     * @param {number} lon - Longitude
     * @param {string} name - Station name
     * @returns {Promise<Response>} Response object
     */
    async setUserLocation(lat, lon, name) {
        const response = await fetch(
            `/api/user-location?lat=${lat}&lon=${lon}&name=${encodeURIComponent(name)}`,
            { method: 'POST' }
        );
        if (!response.ok) {
            throw new Error(`Failed to set location: ${response.statusText}`);
        }
        return response;
    },

    /**
     * Get status message for ESP32/user (distance to tram)
     * @returns {Promise<string>} Status message
     */
    async getStatus() {
        const response = await fetch('/api/status');
        if (!response.ok) {
            throw new Error(`Failed to load status: ${response.statusText}`);
        }
        return await response.text();
    }
};
