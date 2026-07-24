package com.digitaltwin.digitaltwinbackend.service;

import com.digitaltwin.digitaltwinbackend.model.BatteryData;
import org.springframework.stereotype.Service;

import java.util.Random;

@Service
public class BatteryService {

    private final Random random = new Random();

    public BatteryData getBatteryData() {

        int percentage = random.nextInt(101);
        int range = percentage * 3;
        double temperature = 25 + random.nextDouble() * 15;
        int health = 90 + random.nextInt(11);

        return new BatteryData(
                percentage,
                range,
                temperature,
                health
        );
    }
}