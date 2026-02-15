package com.example.YouShouldGo;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

@RestController
public class TramController {
    private final TramOrientationService service;

    @Value("${my_lat}")
    double myLat;
    @Value("${my_lon}")
    double myLon;

    public TramController(TramOrientationService service) {
        this.service = service;
    }

    @GetMapping("/api/map")
    public Map<Integer, TramOrientationService.StopLocation> getMap() {
        return service.getTripMap();
    }
    @GetMapping("/api/vehicles")
    public List<TramOrientationService.Vehicle> getLiveVehicles() {
        return service.getRouteVehicles();
    }
    @GetMapping("/api/status")
    public String getStatus() {


        return service.getTramStatusForESP32(myLat, myLon);
    }
}