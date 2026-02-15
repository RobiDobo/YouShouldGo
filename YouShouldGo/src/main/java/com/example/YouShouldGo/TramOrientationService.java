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

@Service
public class TramOrientationService {

    @Value("${key}")
    String key;
    @Value("${agency}")
    String agency;
    @Value("${targetTrip}")
    String targetTrip;

    public record Stop(Integer stop_id, String stop_name, Double stop_lat, Double stop_lon) {}
    public record StopTime(String trip_id, Integer stop_id, Integer stop_sequence) {}
    public record Vehicle(Integer id, String label, Double latitude, Double longitude, String trip_id, Double speed) {}
    public record StopLocation(String name, Double lat, Double lon, int sequence) {}

    private final RestClient restClient;

    @Getter
    private final Map<Integer, StopLocation> tripMap = new HashMap<>();

    @PostConstruct
    public void init() {
        buildMapForTrip();
    }

    public TramOrientationService(RestClient restClient) {
        this.restClient = restClient;
    }

    public void buildMapForTrip() {
        var spec1 = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/stop_times")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", agency); });

        List<StopTime> allTripTimes = spec1.retrieve().body(new ParameterizedTypeReference<List<StopTime>>() {});

        List<StopTime> filteredTripTimes = allTripTimes.stream()
                .filter(st -> targetTrip.equals(st.trip_id()))
                .toList();

        List<Integer> targetIds = filteredTripTimes.stream()
                .map(StopTime::stop_id).toList();

        var spec2 = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/stops")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", agency); });

        List<Stop> allStops = spec2.retrieve().body(new ParameterizedTypeReference<List<Stop>>() {});

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

    public List<Vehicle> getRouteVehicles() {
        var spec = (RestClient.RequestHeadersSpec<?>) restClient.get()
                .uri("https://api.tranzy.ai/v1/opendata/vehicles")
                .headers(h -> { h.add("X-API-KEY", key); h.add("X-Agency-Id", agency); });

        List<Vehicle> all = spec.retrieve().body(new ParameterizedTypeReference<List<Vehicle>>() {});

        return all.stream()
                .filter(v -> targetTrip.equals(v.trip_id()) && v.latitude() != null && v.longitude() != null)
                .toList();
    }

    public String getTramStatusForESP32(double myLat, double myLon) {
        List<Vehicle> activeTrams = getRouteVehicles();

        // current user station based on input coordinates
        StopLocation userStop = tripMap.values().stream()
                .min(Comparator.comparingDouble(s -> calculateDistance(myLat, myLon, s.lat(), s.lon())))
                .orElse(null);

        if (userStop == null) return "Map Error, hashmap not built";

        Vehicle bestTram = null;
        int minStopsAway = Integer.MAX_VALUE;

        for (Vehicle v : activeTrams) {
            // current tram station based on closest vehicle coordinates
            StopLocation tramStop = tripMap.values().stream()
                    .min(Comparator.comparingDouble(s -> calculateDistance(v.latitude(), v.longitude(), s.lat(), s.lon())))
                    .orElse(null);

            if (tramStop != null) {
                int stopsAway = userStop.sequence() - tramStop.sequence();

                // check if tram is coming
                if (stopsAway >= 0 && stopsAway < minStopsAway) {
                    minStopsAway = stopsAway;
                    bestTram = v;
                }
            }
        }

        if (bestTram != null) {
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
}