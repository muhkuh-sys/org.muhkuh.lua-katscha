Schaltplan ist hier: http://static.hilscher.local/shares/Cad/Fertig/TEST/TESTSYSTEM-24V-IO/7911.050R2/P1602002.pdf

Control pins:

 Name                NRP 52-RE Pin           netX52 Pin
 ENABLE_SOURCE       DPM_D4        (6)       DPM_D4
 ENABLE_SINK         DPM_D5        (7)       DPM_D5

 SPI_CLK             DPM_D8        (10)      DPM_D8
 SPI_CS0             DPM_D9        (11)      DPM_D9
 SPI_CS1             DPM_D10       (12)      DPM_D10
 SPI_CS2             DPM_D11       (13)      DPM_D11
 SPI_CS3             GPIO          (66)      MMIO23
 SPI_MISO            DPM_D12       (14)      DPM_D12
 SPI_MOSI            DPM_D13       (15)      DPM_D13

 I2C_SCL             DPM_D14       (16)      DPM_D14
 I2C_SDA             DPM_D15       (17)      DPM_D15

 SET_PWM             UART_TX/SYNC1 (42)      MMIO21



Drehcodierschalter sind an NRP S1_x und S2_x:

 Name   NRP Pin    Function
 S1_1        50      MMIO00
 S1_2        51      MMIO01
 S1_4        52      MMIO02
 S1_8        53      MMIO03

 S2_1        46      MMIO04
 S2_2        47      MMIO05
 S2_4        48      MMIO06
 S2_8        49      MMIO07
