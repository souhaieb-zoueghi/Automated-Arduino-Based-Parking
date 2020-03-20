# Smart-Parking
## 1. Introduction
This is an academic project developed in the private high school of engineering and technologies (ESPRIT) situated in Tunisia.
its mainly an automated-parking controlled by two arduino uno boards.
## 2. Description
Each arduino board have a specific task, these two cummunicate via the i2c Bus. the first one is the master, it controlls the parking's entrance and communicates with other components, it gets the temperature and time from the DS1621 and DS1307 respectively and then display them on the LCD (i2c), it writes also the statistics (earnings) at the end of every day on the SD card (SPI).
the second board is the slave, it controlls the parking's exit and informs its master about the number of cars that got out from the parking, and depending on the parking's status received from the master it turns on the red led or the green led via the MCP expander (SPI).

## 3. Project architecture
![](images/ProjectArchitecture.jpg)

## 4. State machine
![](images/StateMachine.jpg)
