#pragma once
#pragma once
#include <string>
#include <vector>
#include <SFML/System.hpp>

// 1. DATA MODEL: Represents a single building asset in the park.
struct ParkBuilding {
    uint32_t building_id;          // Unique identifier
    std::string type_id;           // Class or name (e.g., "coffee_shop")
    std::string label;             // Display name
    sf::Vector2i grid_position;    // Flat logical coordinates
    int cost_ip;                   // Inspiration Point cost
    int yield_ip_per_second;       // Passive revenue
    bool active;                   // Current state
};

// 2. PARK MANAGER: The state manager class for simulation logic.
class ParkSystem {
private:
    std::vector<ParkBuilding> buildings; // Structure container
    int currentInspiration;               // Economy balance
    float simulationTimeFactor;           // Simulation speed multiplier

    // INTERNAL METHODS
    void calculateActiveYield();          // Updates IP based on buildings
    void processPedestrianFlow();        // Future use: visitor interactions

public:
    ParkSystem(); // Initializes state

    // PUBLIC INTERFACE: Functions for game interaction
    bool PlaceStructure(const std::string& type, sf::Vector2i gridPos);
    void DemolishStructure(uint32_t id);
    void UpdateTick(float deltaTime);     // Core logic loop
};