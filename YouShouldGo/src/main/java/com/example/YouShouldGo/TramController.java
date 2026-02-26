package com.example.YouShouldGo;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
public class TramController {
    private final TramOrientationService service;


    public TramController(TramOrientationService service) {
        this.service = service;
    }

    @GetMapping("/api/agencies")
    public List<TramOrientationService.Agency> getAgencies() {
        return service.getAgencies();
    }

    @GetMapping("/api/routes")
    public List<TramOrientationService.Route> getRoutes() {
        return service.getRoutes();
    }

    @GetMapping("/api/routes-with-vehicles")
    public List<TramOrientationService.Route> getRoutesWithVehicles() {
        return service.getRoutesWithVehicles();
    }

    @GetMapping("/api/trips")
    public List<TramOrientationService.Trip> getTrips(@RequestParam(required = false) Integer routeId) {
        if (routeId != null) {
            return service.getTripsForRoute(routeId);
        }
        return service.getTrips();
    }
    

    @PostMapping("/api/trips/select")
    public String selectTrip(@RequestParam String tripId) {
        service.setSelectedTrip(tripId);
        return "Selected trip: " + tripId;
    }

    @PostMapping("/api/user-location")
    public String setUserLocation(@RequestParam double lat, @RequestParam double lon, @RequestParam(required = false) String name) {
        service.setUserLocation(lat, lon, name);
        return name != null ? "User location set to: " + name : "User location set";
    }

    @GetMapping("/api/trips/selected")
    public String getSelectedTrip() {
        return service.getSelectedTrip();
    }

    @GetMapping("/api/map")
    public Map<Integer, TramOrientationService.StopLocation> getMap() {
        return service.getTripMap();
    }
    @GetMapping("/api/vehicles")
    public List<TramOrientationService.Vehicle> getLiveVehicles() {
        return service.getRouteVehicles();
    }
    
    @GetMapping("/api/stations-with-vehicles")
    public List<TramOrientationService.StationWithVehicle> getStationsWithVehicles() {
        return service.getStationsWithVehicles();
    }
    
    @GetMapping("/api/status")
    public String getStatus() {
        return service.getTramStatusForESP32();
    }
}