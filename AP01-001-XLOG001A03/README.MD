# SOFTWARE Aquarium Potager by Nénufarm

## Description
This project is designed to manage the operation of the Nénufarm "Aquarium Potager". It is based on an ATtiny3217 microcontroller and developed using Microchip Studio.
Each component of the Smart Aquarium is represented as a C++ class, enabling modular management of system elements:
- Pump
- Aquarium light
- Plant light
- Memory
- Buttons

The system's intelligence is managed by a Handler, which orchestrates the components. There are two versions:
- **HandlerClient**: Default version running on the client’s system.
- **HandlerProd**: Version used for maintenance and production.
The **StartManager** determines the appropriate Handler at startup.

### Main Functions
- **everyTime()**: Manages external events (e.g., button presses).
- **everyMilSecond()**: Manages internal components.
- **everySecond()**: Handles scheduled events via the Scheduler.

## License
This project is licensed under the **MIT License**.