package com.digitaltwin.digitaltwinbackend.model;

public class BatteryData {

    private int batteryPercentage;
    private int remainingRange;
    private double temperature;
    private int batteryHealth;

    public BatteryData() {
    }

    public BatteryData(int batteryPercentage, int remainingRange,
                       double temperature, int batteryHealth) {
        this.batteryPercentage = batteryPercentage;
        this.remainingRange = remainingRange;
        this.temperature = temperature;
        this.batteryHealth = batteryHealth;
    }

    public int getBatteryPercentage() {
        return batteryPercentage;
    }

    public void setBatteryPercentage(int batteryPercentage) {
        this.batteryPercentage = batteryPercentage;
    }

    public int getRemainingRange() {
        return remainingRange;
    }

    public void setRemainingRange(int remainingRange) {
        this.remainingRange = remainingRange;
    }

    public double getTemperature() {
        return temperature;
    }

    public void setTemperature(double temperature) {
        this.temperature = temperature;
    }

    public int getBatteryHealth() {
        return batteryHealth;
    }

    public void setBatteryHealth(int batteryHealth) {
        this.batteryHealth = batteryHealth;
    }
}