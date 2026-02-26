package com.example.YouShouldGo;

import jakarta.annotation.PostConstruct;
import lombok.Getter;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestClient;

import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

@Service
public class TramOrientationService {

    @Value("${key}")
    String key; //api key from env

    @Getter
    private String selectedTrip; // Currently selected trip by user
    
    @Getter
    private String selectedAgency = "2"; // Hardcodat to Cluj (agency_id=2) - API key restriction

    private Double userLat;
    private Double userLon;
    @Getter
    private String userStopName;

    public record Agency(Integer agency_id, String agency_name, String agency_url, String agency_timezone) {} //api data: transit agency information
    public record Route(Integer route_id, String route_short_name, String route_long_name, Integer route_type) {} //api data: route information (e.g., "7" tram line)
    public record Trip(String trip_id, Integer route_id, Integer direction_id, String trip_headsign) {} //api data: specific trip with direction (e.g., "7 tram to downtown")
    public record Stop(Integer stop_id, String stop_name, Double stop_lat, Double stop_lon) {} //api data: stop metadata: ID, name, and geolocation
    public record StopTime(String trip_id, Integer stop_id, Integer stop_sequence) {} //api data: maps trip IDs to stop IDs with their sequence order (1st, 2nd, 3rd station, etc.)
    public record Vehicle(Integer id, String label, Double latitude, Double longitude, String trip_id, Double speed) {}//api data: real-time vehicle positions with lat/lon and which trip they're on
    public record StopLocation(String name, Double lat, Double lon, int sequence) {}//agregate stop info for easy access: name, geolocation, and sequence in the trip
    public record StationWithVehicle(int sequence, String stationName, Double lat, Double lon, List<VehicleInfo> vehicles, int hasVehicle) {}
    public record VehicleInfo(Integer id, String label, Double speed) {}

    private final RestClient restClient;

    @Getter
    private final Map<Integer, StopLocation> tripMap = new HashMap<>();

    public TramOrientationService(RestClient restClient) {
        this.restClient = restClient;
    }
    //ordered list of all stops on the route with their physical locations.
    public void buildMapForTrip(String tripId) {
        if (selectedAgency == null || selectedAgency.isEmpty()) {
            throw new IllegalStateException("Please select an agency first using POST /api/agencies/select?agencyId=X");
        }
        tripMap.clear(); // Clear existing data
        var spec1 = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/stop_times")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", selectedAgency); });

        List<StopTime> allTripTimes = spec1.retrieve().body(new ParameterizedTypeReference<List<StopTime>>() {});

        List<StopTime> filteredTripTimes = allTripTimes.stream()
                .filter(st -> tripId.equals(st.trip_id()))
                .toList();
        // extract stop IDs for the target trip (19_1 for example which is the 7 tram in the returning direction)

        List<Integer> targetIds = filteredTripTimes.stream()
                .map(StopTime::stop_id).toList();

        var spec2 = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/stops")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", selectedAgency); });

        List<Stop> allStops = spec2.retrieve().body(new ParameterizedTypeReference<List<Stop>>() {});
        // extract stop data for the target stop IDs and build the map with stop name, geolocation, and sequence in the trip
        for (Integer id : targetIds) { 
            int sequence = filteredTripTimes.stream()
                    .filter(st -> st.stop_id().equals(id))
                    .findFirst()
                    .map(StopTime::stop_sequence)
                    .orElse(0);

            allStops.stream()
                    .filter(s -> s.stop_id().equals(id))
                    .findFirst()
                    .ifPresent(s -> tripMap.put(id, new StopLocation(
                            s.stop_name(),
                            s.stop_lat(),
                            s.stop_lon(),
                            sequence
                    )));
        }
    }

    public List<Agency> getAgencies() {
        var spec = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/agency")
                .headers(h -> h.add("X-API-KEY", key));

        List<Agency> allAgencies = spec.retrieve().body(new ParameterizedTypeReference<List<Agency>>() {});
        
        // Filter to only Cluj (agency_id=2) - API key restriction
        return allAgencies.stream()
                .filter(agency -> agency.agency_id() == 2)
                .toList();
    }

    public List<Route> getRoutes() {
        if (selectedAgency == null || selectedAgency.isEmpty()) {
            throw new IllegalStateException("Please select an agency first using POST /api/agencies/select?agencyId=X");
        }
        var spec = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/routes")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", selectedAgency); });

        return spec.retrieve().body(new ParameterizedTypeReference<List<Route>>() {});
    }

    public List<Route> getRoutesWithVehicles() {
        if (selectedAgency == null || selectedAgency.isEmpty()) {
            throw new IllegalStateException("Please select an agency first");
        }

        // Get all vehicles
        var vehicleSpec = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/vehicles")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", selectedAgency); });
        List<Vehicle> allVehicles = vehicleSpec.retrieve().body(new ParameterizedTypeReference<List<Vehicle>>() {});

        // Get all trips to map trip_id to route_id
        List<Trip> allTrips = getTrips();
        Map<String, Integer> tripToRoute = new HashMap<>();
        allTrips.forEach(trip -> tripToRoute.put(trip.trip_id(), trip.route_id()));

        // Get route IDs that have active vehicles
        Set<Integer> routeIdsWithVehicles = allVehicles.stream()
                .filter(v -> v.trip_id() != null && v.latitude() != null && v.longitude() != null)
                .map(v -> tripToRoute.get(v.trip_id()))
                .filter(routeId -> routeId != null)
                .collect(Collectors.toSet());

        // Get all routes and filter to only those with vehicles
        List<Route> allRoutes = getRoutes();
        return allRoutes.stream()
                .filter(route -> routeIdsWithVehicles.contains(route.route_id()))
                .toList();
    }

    public List<Trip> getTrips() {
        if (selectedAgency == null || selectedAgency.isEmpty()) {
            throw new IllegalStateException("Please select an agency first using POST /api/agencies/select?agencyId=X");
        }
        var spec = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/trips")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", selectedAgency); });

        return spec.retrieve().body(new ParameterizedTypeReference<List<Trip>>() {});
    }

    public List<Trip> getTripsForRoute(Integer routeId) {
        List<Trip> allTrips = getTrips();
        return allTrips.stream()
                .filter(trip -> trip.route_id().equals(routeId))
                .toList();
    }

    public void setSelectedTrip(String tripId) {
        this.selectedTrip = tripId;
        this.userLat = null;
        this.userLon = null;
        this.userStopName = null;
        // Rebuild the map for the new trip
        buildMapForTrip(tripId);
    }

    public void setUserLocation(double lat, double lon, String stopName) {
        this.userLat = lat;
        this.userLon = lon;
        this.userStopName = stopName;
    }

    public List<Vehicle> getRouteVehicles() {
        if (selectedAgency == null || selectedAgency.isEmpty()) {
            throw new IllegalStateException("Please select an agency first using POST /api/agencies/select?agencyId=X");
        }
        
        if (selectedTrip == null || selectedTrip.isEmpty()) {
            return List.of(); // Return empty list if no trip selected yet
        }
        
        var spec = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/vehicles")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", selectedAgency); });

        List<Vehicle> all = spec.retrieve().body(new ParameterizedTypeReference<List<Vehicle>>() {});

        // Use selectedTrip if set
        String activeTripId = selectedTrip;
        return all.stream()
                .filter(v -> activeTripId.equals(v.trip_id()) && v.latitude() != null && v.longitude() != null)
                .toList();
    }

    public String getTramStatusForESP32() {
        if (selectedTrip == null || selectedTrip.isEmpty()) {
            return "Please select a trip first";
        }
        
        if (userLat == null || userLon == null) {
            return "Please select your station first";
        }

        // Get all stations with their vehicle positions
        List<StationWithVehicle> stations = getStationsWithVehicles();
        
        if (stations.isEmpty()) {
            return "Map Error, hashmap not built";
        }

        // Find user's current station
        StationWithVehicle userStation = stations.stream()
            .min(Comparator.comparingDouble(s -> 
                calculateDistance(userLat, userLon, s.lat(), s.lon())))
            .orElse(null);

        if (userStation == null) {
            return "Map Error, hashmap not built";
        }

        // Find closest tram coming towards user
        int minStopsAway = Integer.MAX_VALUE;

        for (StationWithVehicle station : stations) {
            if (station.hasVehicle() == 1) {
                int stopsAway = userStation.sequence() - station.sequence();
                
                // Check if tram is coming (before user's station)
                if (stopsAway >= 0 && stopsAway < minStopsAway) {
                    minStopsAway = stopsAway;
                }
            }
        }

        if (minStopsAway != Integer.MAX_VALUE) {
            if (minStopsAway == 1) return "Next Stop";
            if (minStopsAway == 0) return "In Station";
            return minStopsAway + " stops away";
        }

        return "all vehicles passed your stop";
    }

    private double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
        final int R = 6371;
        double dLat = Math.toRadians(lat2 - lat1);
        double dLon = Math.toRadians(lon2 - lon1);
        double a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
                Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2)) *
                        Math.sin(dLon / 2) * Math.sin(dLon / 2);
        return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    }

    public List<StationWithVehicle> getStationsWithVehicles() {
        if (selectedTrip == null || selectedTrip.isEmpty() || tripMap.isEmpty()) {
            return List.of(); // Return empty list if no trip selected yet
        }
        
        List<Vehicle> vehicles = getRouteVehicles();
        
        // Map each vehicle to its closest station
        Map<Integer, List<VehicleInfo>> vehiclesByStation = new HashMap<>();
        
        for (Vehicle vehicle : vehicles) {
            StopLocation closestStop = tripMap.values().stream()
                .min(Comparator.comparingDouble(s -> 
                    calculateDistance(vehicle.latitude(), vehicle.longitude(), s.lat(), s.lon())))
                .orElse(null);
            
            if (closestStop != null) {
                vehiclesByStation.computeIfAbsent(closestStop.sequence(), k -> new java.util.ArrayList<>())
                    .add(new VehicleInfo(vehicle.id(), vehicle.label(), vehicle.speed()));
            }
        }
        
        // Create StationWithVehicle for each station
        return tripMap.values().stream()
            .sorted(Comparator.comparingInt(StopLocation::sequence))
            .map(stop -> {
                List<VehicleInfo> stationVehicles = vehiclesByStation.getOrDefault(stop.sequence(), List.of());
                int hasVehicle = stationVehicles.isEmpty() ? 0 : 1;
                return new StationWithVehicle(
                    stop.sequence(), 
                    stop.name(), 
                    stop.lat(), 
                    stop.lon(), 
                    stationVehicles,
                    hasVehicle
                );
            })
            .toList();
    }
}