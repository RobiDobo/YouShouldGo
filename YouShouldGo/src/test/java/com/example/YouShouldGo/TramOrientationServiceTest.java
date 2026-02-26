package com.example.YouShouldGo;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.test.context.SpringBootTest;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Regression tests for TramOrientationService.
 * These tests verify core business logic, data structures, and safe operations.
 * 
 * For integration testing with live API, set the API key in:
 * - Run Configuration: VM options: -Dkey=your-api-key
 * - Or application.properties
 */
@SpringBootTest
class TramOrientationServiceTest {

    @Autowired
    private TramOrientationService service;

    @Value("${key:test-key}")
    private String apiKey;

    @Test
    void testServiceLoads() {
        // Verify the service can be autowired
        assertNotNull(service);
    }

    @Test
    void testApiKeyLoaded() {
        // Verify API key is injected from configuration
        assertNotNull(apiKey);
        System.out.println("API Key loaded: " + (apiKey.equals("test-key") ? "test-key (default)" : "from config"));
    }

    @Test
    void testGetSelectedAgency_ReturnsCluj() {
        // The service is hardcoded to Cluj (agency_id=2)
        assertEquals("2", service.getSelectedAgency());
    }

    @Test
    void testGetStationsWithVehicles_ReturnsEmptyWhenNoTripSelected() {
        // When no trip is selected, should return empty list
        List<TramOrientationService.StationWithVehicle> result = service.getStationsWithVehicles();
        
        assertNotNull(result);
        assertTrue(result.isEmpty());
    }

    @Test
    void testSetUserLocation_StoresLocationCorrectly() {
        // Set user location
        service.setUserLocation(46.77, 23.59, "Piață Unirii");
        
        // Verify it's stored
        assertEquals("Piață Unirii", service.getUserStopName());
    }

    @Test
    void testRecordTypes_Agency_CanBeCreated() {
        TramOrientationService.Agency agency = new TramOrientationService.Agency(
            2, "Cluj-Napoca", "http://example.com", "Europe/Bucharest"
        );
        
        assertEquals(2, agency.agency_id());
        assertEquals("Cluj-Napoca", agency.agency_name());
        assertEquals("http://example.com", agency.agency_url());
        assertEquals("Europe/Bucharest", agency.agency_timezone());
    }

    @Test
    void testRecordTypes_Route_CanBeCreated() {
        TramOrientationService.Route route = new TramOrientationService.Route(
            1, "7", "Tramvai 7", 0
        );
        
        assertEquals(1, route.route_id());
        assertEquals("7", route.route_short_name());
        assertEquals("Tramvai 7", route.route_long_name());
        assertEquals(0, route.route_type());
    }

    @Test
    void testRecordTypes_Trip_CanBeCreated() {
        TramOrientationService.Trip trip = new TramOrientationService.Trip(
            "trip_123", 1, 0, "line end"
        );
        
        assertEquals("trip_123", trip.trip_id());
        assertEquals(1, trip.route_id());
        assertEquals(0, trip.direction_id());
        assertEquals("line end", trip.trip_headsign());
    }

    @Test
    void testRecordTypes_Vehicle_CanBeCreated() {
        TramOrientationService.Vehicle vehicle = new TramOrientationService.Vehicle(
            101, "T-101", 46.77, 23.59, "trip_123", 30.0
        );
        
        assertEquals(101, vehicle.id());
        assertEquals("T-101", vehicle.label());
        assertEquals(46.77, vehicle.latitude());
        assertEquals(23.59, vehicle.longitude());
        assertEquals("trip_123", vehicle.trip_id());
        assertEquals(30.0, vehicle.speed());
    }

    @Test
    void testRecordTypes_StopLocation_CanBeCreated() {
        TramOrientationService.StopLocation stopLoc = new TramOrientationService.StopLocation(
            "Piața Unirii", 46.77, 23.59, 1
        );
        
        assertEquals("Piața Unirii", stopLoc.name());
        assertEquals(46.77, stopLoc.lat());
        assertEquals(23.59, stopLoc.lon());
        assertEquals(1, stopLoc.sequence());
    }

    @Test
    void testRecordTypes_VehicleInfo_CanBeCreated() {
        TramOrientationService.VehicleInfo vehicleInfo = new TramOrientationService.VehicleInfo(
            101, "T-101", 30.0
        );
        
        assertEquals(101, vehicleInfo.id());
        assertEquals("T-101", vehicleInfo.label());
        assertEquals(30.0, vehicleInfo.speed());
    }

    @Test
    void testRecordTypes_StationWithVehicle_CanBeCreated() {
        List<TramOrientationService.VehicleInfo> vehicles = List.of(
            new TramOrientationService.VehicleInfo(101, "T-101", 30.0)
        );
        
        TramOrientationService.StationWithVehicle station = new TramOrientationService.StationWithVehicle(
            1, "Piața Unirii", 46.77, 23.59, vehicles, 1
        );
        
        assertEquals(1, station.sequence());
        assertEquals("Piața Unirii", station.stationName());
        assertEquals(46.77, station.lat());
        assertEquals(23.59, station.lon());
        assertEquals(1, station.vehicles().size());
        assertEquals(1, station.hasVehicle());
    }

    @Test
    void testGetStationsWithVehicles_SafelyHandlesNoData() {
        // When no trip is selected and no data available,
        // should return empty list without throwing exception
        List<TramOrientationService.StationWithVehicle> stations = service.getStationsWithVehicles();
        
        assertNotNull(stations);
        assertTrue(stations.isEmpty());
    }
}
