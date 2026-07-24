package com.digitaltwin.digitaltwinbackend.controller;

import com.digitaltwin.digitaltwinbackend.model.BatteryData;
import com.digitaltwin.digitaltwinbackend.service.BatteryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/battery")
@CrossOrigin(origins = "*")
public class BatteryController {

    @Autowired
    private BatteryService batteryService;

    @GetMapping
    public BatteryData getBatteryData() {
        return batteryService.getBatteryData();
    }
}