/*
 * Project Name: Radio Firmware
 * File: BK4819.cpp
 *
 * Copyright (C) 2024 Fabrizio Palumbo (IU0IJV)
 * 
 * This program is distributed under the terms of the MIT license.
 * You can obtain a copy of the license at:
 * https://opensource.org/licenses/MIT
 *
 * DESCRIPTION:
 * Library implementation for interfacing with the Beken BK4819 radio module.
 *
 * AUTHOR: Fabrizio Palumbo
 * CREATION DATE: October 27, 2024
 *
 * CONTACT: t.me/IU0IJV
 *
 * NOTES:
 * - This implementation includes functions for initializing and controlling the BK4819 module.
 * - Verify correct SPI pin configuration before use.
 */


#include "BK4819.h"

// ---------------------------------------------------- Configurazione tabella Gain
const t_gain_table gain_table[] =
{                      //   Idx      dB				LNAS	LNAG	MIX		PGA
	{0x10	,90},      //	0 	  	-57   			0		0		2		0		
	{0x1	,88},      //	1 		-55       		0		0	    0	    1
	{0x9	,87},      //	2  		-54      		0		0	    1	    1
	{0x2	,83},      //	3  		-50       		0		0	    0	    2
	{0xA	,81},      //	4  		-48       		0		0	    1	    2
	{0x12	,79},      //	5  		-46       		0		0	    2	    2
	{0x2A	,77},      //	6  		-44       		0		1	    1	    2
	{0x32	,75},      //	7  		-42       		0		1	    2	    2
	{0x3A	,70},      //	8  		-37       		0		1	    3	    2
	{0x20B	,68},      //   9  		-35       		2		0	    1	    3
	{0x213	,64},      //   10 		-31      		2		0	    2	    3
	{0x21B	,62},      //   11 		-29       		2		0	    3	    3
	{0x214	,59},      //   12 		-26       		2		0	    2	    4
	{0x21C	,56},      //   13 		-23      		2		0	    3	    4
	{0x22D	,52},      //   14 		-19      		2		1	    1	    5
	{0x23C	,50},      //   15 		-17      		2		1	    3	    4
	{0x23D	,48},      //   16 		-15      		2		1	    3	    5
	{0x255	,44},      //   17 		-11      		2		2	    2	    5
	{0x25D	,42},      //   18 		-9      		2		2	    3	    5
	{0x275	,39},      //   19 		-6       		2		3	    2	    5
	{0x295	,33},      //   20 		 0       		2		4	    2	    5
	{0x2B6	,31},      //   21 		+2        		2		4	    2	    6
	{0x354	,28},      //   22 		+5 	   			3		2	    2	    4
	{0x36C	,23},      //   23 		+9       		3		3	    1	    4
	{0x38C	,20},      //   24 		+13        		3		4	    1	    4
	{0x38D	,17},      //   25 		+16        		3		4	    1	    5
	{0x3B5	,13},      //   26 		+20        		3		5	    2	    5
	{0x3B6	,9 },      //   27 		+22      		3		5	    2	    6
	{0x3D6	,8 },      //   28 		+25       		3		6	    2	    6
	{0x3BF	,3 },      //   29 		+28      		3		5		3		7					
	{0x3DF	,2 },      //   30 		+31      		3		6		3		7
	{0x3FF	,0 },      //   31 		+33  			3		7		3		7
};

t_Vfo_Data VfoD;

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************

volatile bool spiInUse = false;

// ---------------------------------------------------- Configurazione Libreria
BK4819::BK4819(int csPin, int MosiPin, int MisoPin, int sckPin) 
{
    _csPin = csPin;
    _MosiPin = MosiPin;
    _MisoPin = MisoPin;
    _sckPin = sckPin;
												// Configura i pin
    pinMode(_csPin, OUTPUT);					// Metti il pin in modalità output per la scrittura
    pinMode(_sckPin, INPUT);					// Metti il pin in modalità input per la lettura	
	pinMode(_MosiPin, INPUT);   				// Metti il pin in modalità input per la lettura	
	pinMode(_MisoPin, INPUT);   				// Metti il pin in modalità input per la lettura	
	
	digitalWrite(_sckPin, HIGH);  				// Imposta il clock alto inizialmente (modalità 3)
    digitalWrite(_csPin, HIGH);   				// Disabilita inizialmente il chip select
    
    SPI.begin();                 				// Inizializza SPI
    SPI.setClockDivider(SPI_CLOCK_DIV64); 		// Imposta la velocità SPI 250 Khz ( non aumentare la velocita' )
}

void BK4819::BK4819_SCN_select( uint8_t csPin )
{
	_csPin = csPin;
	pinMode(_csPin, OUTPUT);					// Metti il pin in modalità output per la scrittura
	digitalWrite(_csPin, HIGH);   				// Disabilita inizialmente il chip select
}


// ---------------------------------------------------- Scrive in un registro del BK4819
void BK4819::BK4819_Write_Register(uint16_t address, uint16_t data) 
{
												// Definisci il timeout in microsecondi
    const unsigned long timeout = 100000UL; 	// 10 millisecondi
    unsigned long startTime = micros();
												// Attendi il lock con timeout
    while (spiInUse) 
	{
        if ((micros() - startTime) > timeout) 
		{
			spiInUse = false;					// Rilascia il lock
            return;  							// Esci dalla funzione per evitare stallo
        }
    }
	
    spiInUse = true;							// Prendi il lock
	
    pinMode(_MosiPin, OUTPUT);  		
	pinMode(_sckPin, OUTPUT);
												// 1. Abbassa manualmente il clock prima di abbassare il CS
    digitalWrite(_sckPin, LOW);
												// 2. Ora abbassa il CS per iniziare la comunicazione
    digitalWrite(_csPin, LOW);      			// Seleziona il chip
	delayMicroseconds(3); 
												// 3. Invia i dati SPI (usando modalità 0)
	SPI.setDataMode(SPI_MODE0);
	
    SPI.transfer(address & 0x7F);   			// Bit di scrittura
    SPI.transfer16(data);           			// Scrivi i 16 bit
												// 4. Invia il fine della comunicazione
    digitalWrite(_csPin, HIGH);     			// Rilascia il chip
    pinMode(_MosiPin, INPUT);   				// Metti il pin in modalità input per la lettura
	pinMode(_sckPin, INPUT);  					// Imposta il clock alto inizialmente (modalità 3)
	
    spiInUse = false;							// Rilascia il lock
}


// ---------------------------------------------------- Legge da un registro del BK4819
uint16_t BK4819::BK4819_Read_Register(uint16_t address) 
{
												// Definisci il timeout in microsecondi
    const unsigned long timeout = 100000UL; 	// 100 millisecondi
    unsigned long startTime = micros();
												// Attendi il lock con timeout
    while (spiInUse) 
	{
        if ((micros() - startTime) > timeout) 
		{
			spiInUse = false;					// Rilascia il lock
            return 0;  							// Esci dalla funzione per evitare stallo
        }
    }
	
    spiInUse = true;							// Prendi il lock
	
    pinMode(_MosiPin, OUTPUT);  				// Imposta il pin in modalità output per l'invio dell'indirizzo
	pinMode(_sckPin, OUTPUT);
												// 1. Abbassa manualmente il clock prima di abbassare il CS
    digitalWrite(_sckPin, LOW);
												// 2. Ora abbassa il CS per iniziare la comunicazione
    digitalWrite(_csPin, LOW);      			// Seleziona il chip
	delayMicroseconds(3); 
												// 3. Invia i dati SPI (usando modalità 0)
	SPI.setDataMode(SPI_MODE0);
		
    SPI.transfer(address | 0x80);   			// Bit di lettura
    pinMode(_MosiPin, INPUT);   				// Metti il pin in modalità input per la lettura
    
    uint16_t result = SPI.transfer16(0x0000);  	// Leggi i 16 bit
												// 4. Invia il fine della comunicazione
    digitalWrite(_csPin, HIGH);     			// Rilascia il chip
    pinMode(_MosiPin, INPUT);   				// Metti il pin in modalità input per la lettura
	pinMode(_sckPin, INPUT);  					// Imposta il clock alto inizialmente (modalità 3)
	
	spiInUse = false;							// Rilascia il lock
	
    return result;
}

// ******************************************************************************************************************************
//
// ******************************************************************************************************************************
// ---------------------------------------------------- Funzione di inizializzazione
void BK4819::BK4819_Init() 
{
    BK4819_SoftReset();							// Resetta il Beken
	
	BK4819_Write_Register(0x30, 0);				// Spegne il beken
	
	BK4819_Write_Register(0x33, 0xEFFF);  		// Configurazione GPIO subito dopo il reset per non avere uscite esposte.
												// <15>	?		1: Output disable / 0: Output enable
												// <14> GPIO6	1
												// <13> GPIO5	1
												// <12> GPIO4 *	0 ->enable out
												
												// <11> GPIO3	1
												// <10> GPIO2	1
												// <9>  GPIO1	1	
												// <8>  GPIO0	1
												
												// <7> 	?		1: High when output is enabled / 0: Low when output is enabled
												// <6>  GPIO6	1
												// <5>  GPIO5	1
												// <4>  GPIO4	1
												
												// <3>  GPIO3	1
												// <2>  GPIO2	1
												// <1>  GPIO1	1
												// <0>  GPIO0	1

												// GPIO x 
												// 0: High/Low
												// 1: Interrupt
												// 2: Squelch 
												// 3: VOX
												// 4: CTCSS/CDCSS comparison results
												// 5: CTCSS comparison results
												// 6: CDCSS comparison results
												// 7: Tail detection results
												// 8: DTMF/5 Tone symbol received flag
												// 9: CTCSS/CDCSS digital wave
	
	BK4819_Write_Register(0x35,0x0001);			// <15:12>	-----									
	                                            // <11:08>	GPIO6
	                                            // <07:04>	GPIO5
	                                            // <03:00>	GPIO4
						
	BK4819_Write_Register(0x34,0x0000);			// <15:12>	GPIO3
												// <11:08>	GPIO2
												// <07:04>	GPIO1
												// <03:00>	GPIO0
	
	BK4819_Set_AF(AF_MUTE);						// Silenzia l'audio
	
	BK4819_RF_Set_Agc(0);						// Inizializza l'AGC (Automatic Gain Control)
	BK4819_Set_AGC_Gain(AGC_MAN, 0);			// azzera il gain 
	BK4819_Set_Squelch (255,255, 127,127, 127, 127); // setup Squelch al massimo valore
	
	BK4819_Clear_Interrupt();					// cancella gli eventuali interrupt pendenti
	
	BK4819_Set_Xtal(XTAL26M);

	BK4819_Write_Register(0x36, 0x0000);  		// Registro 36: configurazione dell'amplificatore di potenza (PA) Bias = 0V
												// <15:8>	Bias output 0-3.2V
												// <7>		Enable PA Ctl out
												// <5:3>	Gain 1	 0-7
												// <2:0>	Gain 2	 0-7
	
	BK4819_Write_Register(0x3E, 0xA037);		// Band selection threshold ( VCO max. frequency (Hz)/96/640 )
	
												// Configurazione del livello AF (Audio Frequency)
    BK4819_Write_Register(0x48, (11u << 12)|    // ??? 0..15 (riservato)
                                (0u << 10) |    // AF Rx Gain-1 (0 = 0dB)
                                (40u << 4) |    // AF Rx Gain-2 (-26dB ~ 5.5dB, 0.5dB/step)
                                (10u << 0));    // AF DAC Gain (2dB/step, 15 = max, 0 = min)
								
	BK4819_Write_Register(0x49, 0x2A38);		// RF AGC threshold
												// <15:14>  High/Low Lo selection:
												// 			0X: Auto High/Low Lo
												// 			10: Low Lo
												// 			11: High Lo
												// <13:7>	AGC high threshold, 1 dB/LSB
												// <6:0>	RF AGC low threshold, 1 dB/LSB

    BK4819_Write_Register(0x1F, 0x5454);  		// Configurazione del registro RF 

	BK4819_Set_AFC(2);							// AFC abilitato
	
	
	// BK4819_Write_Register(0x1A, 0x7850);
	
	// BK4819_Write_Register(0x1B, 0x2200);		// impostazioni di default
	
	// BK4819_Write_Register(0x36, 0x0008);
	// BK4819_Write_Register(0x37, 0x1F0F);
	// BK4819_Write_Register(0x3B, 0x5880);
	// BK4819_Write_Register(0x3F, 0x040C);
	// BK4819_Write_Register(0x40, 0x36A4);
	// BK4819_Write_Register(0x43, 0x35E8);
	// BK4819_Write_Register(0x4D, 0xA032);
	// BK4819_Write_Register(0x4E, 0x4C1F);
	// BK4819_Write_Register(0x4F, 0x4543);
	// BK4819_Write_Register(0x51, 0x904A);
	// BK4819_Write_Register(0x5F, 0x5940);
	// BK4819_Write_Register(0x61, 0x0633);
	// BK4819_Write_Register(0x62, 0x202B);
	
	// BK4819_Write_Register(0x73, 0x3000); 
	// BK4819_Write_Register(0x78, 0x1917);
	// BK4819_Write_Register(0x7C, 0x595E);
	// BK4819_Write_Register(0x7E, 0x37EE);
	
}

// ---------------------------------------------------- Soft Reset
void BK4819::BK4819_SoftReset() 
{
    BK4819_Write_Register(0, 0x8000);  			// Chip reset
	BK4819_Write_Register(0, 0x0000);  			// normale
}

// ---------------------------------------------------- Spegne il modulo radio
void BK4819::BK4819_RX_TurnOff(void)
{
	BK4819_Write_Register(0x30, 0);
}

// ---------------------------------------------------- Accensione radio in RX
void BK4819::BK4819_RX_TurnOn(void)
{
	BK4819_Write_Register(0x36, 0x0000);
	
	// DSP Voltage Setting = 1			1 
	
	// ANA LDO = 2.7v					1
	// VCO LDO = 2.7v					1
	// RF LDO  = 2.7v					1
	// PLL LDO = 2.7v					1
	
	// ANA LDO bypass					0
	// VCO LDO bypass					0
	// RF LDO  bypass					0
	// PLL LDO bypass					0
	
	// Reserved bit is 1 instead of 0	1
	// Enable  DSP						1
	// Enable  XTAL						1
	// Enable  Band Gap					1
	//
	BK4819_Write_Register(0x37, 0x1F0F);  	

	// Turn off everything
	BK4819_Write_Register(0x30, 0);		

	// 15	Enable  VCO Calibration			1	
	// 10:13Enable  RX Link					1111
	// 9	Enable  AF DAC					1
	// 8	DISC MODE						1
	// 4:7	Enable  PLL/VCO					1111
	// 3	Disable PA Gain					0
	// 2	Disable MIC ADC					0
	// 1	Disable TX DSP					0
	// 0	Enable  RX DSP					1
	//		                                           15     10 9 8    4 3 2 1 0
	BK4819_Write_Register(0x30, 0b1011111111110001); // 1 0 1111 1 1 1111 0 0 0 1
}


// ---------------------------------------------------- Set filtri di banda
void BK4819::BK4819_Set_Filter_Bandwidth(const BK4819_Filter_Bandwidth_t bandwidth) 
{
    const uint8_t bw = bandwidth;

    if (bw > 9) return;
	
	// REG_43
	// <15>    0 ???
	//
	
	const uint8_t rf[] = {7,5,4,3,2,1,3,1,1,0};
	
	// <14:12> 4 RF filter bandwidth
	//         0 = 1.7  KHz
	//         1 = 2.0  KHz
	//         2 = 2.5  KHz
	//         3 = 3.0  KHz *W
	//         4 = 3.75 KHz *N
	//         5 = 4.0  KHz
	//         6 = 4.25 KHz
	//         7 = 4.5  KHz
	// if <5> == 1, RF filter bandwidth * 2
	
	const uint8_t wb[] = {6,4,3,2,2,1,2,1,0,0};
	
	// <11:9>  0 RF filter bandwidth when signal is weak
	//         0 = 1.7  KHz *WN
	//         1 = 2.0  KHz 
	//         2 = 2.5  KHz
	//         3 = 3.0  KHz 
	//         4 = 3.75 KHz
	//         5 = 4.0  KHz
	//         6 = 4.25 KHz
	//         7 = 4.5  KHz
	// if <5> == 1, RF filter bandwidth * 2

	const uint8_t af[] = {4,5,6,7,0,0,3,0,2,1};  

	// <8:6>   1 AFTxLPF2 filter Band Width
	//         1 = 2.5  KHz (for 12.5k channel space) *N
	//         2 = 2.75 KHz
	//         0 = 3.0  KHz (for 25k   channel space) *W
	//         3 = 3.5  KHz
	//         4 = 4.5  KHz
	//         5 = 4.25 KHz
	//         6 = 4.0  KHz
	//         7 = 3.75 KHz

	const uint8_t bs[] = {2,2,2,2,2,2,0,0,1,1}; 

	// <5:4>   0 BW Mode Selection
	//         0 = 12.5k
	//         1 =  6.25k
	//         2 = 25k/20k
	//
	// <3>     1 ???
	//
	// <2>     0 Gain after FM Demodulation
	//         0 = 0dB
	//         1 = 6dB
	//
	// <1:0>   0 ???

    const uint16_t val = (0u 	 << 15) |     //  0
                         (rf[bw] << 12) |     // *3 RF filter bandwidth
                         (wb[bw] <<  9) |     // *0 RF filter bandwidth when signal is weak
                         (af[bw] <<  6) |     // *0 AFTxLPF2 filter Band Width
                         (bs[bw] <<  4) |     //  2 BW Mode Selection 25K
                         (1u 	 <<  3) |     //  1
                         (0u 	 <<  2) |     //  0 Gain after FM Demodulation
                         (0u 	 <<  0);      //  0

    BK4819_Write_Register(0x43, val);
}


// ---------------------------------------------------- Imposta il registro AF
void BK4819::BK4819_Set_AF(AF_Type_t af) 
{
	VfoD.AFmode = af;
	
	// AF Output Inverse Mode = Inverse
	// Undocumented bits 0x4040
	//
	
	// REG_47
	// ----------------------------
	// <15>			?
	// <14>	  0x1	?
	// <13>   0x1   AF output inverse mode: 1: Inverse
	// <12>			?
	// <11:8> 0x1   AF output selection:

	// 				AF_MUTE      =  0u,  // Mute
	// 				AF_FM        =  1u,  // FM
	// 				AF_TONE      =  2u,  // Tone
	//				AF_BEEP      =  3u,  // Beep for TX
	// 				AF_RAW	     =  4u,  // raw (ssb without if filter = raw in sdr sharp)
	// 				AF_DSB 	     =  5u,  // dsb (or ssb = lsb and usb at the same time)
	// 				AF_CTCOUT    =  6u,  // ctcss/dcs (fm with narrow filters for ctcss/dcs)
	// 				AF_AM        =  7u,  // AM
	// 				AF_FSKOUT    =  8u,  // fsk out test with special fsk filters (need reg58 fsk on to give sound on speaker )
	// 				AF_BYPASS    =  9u,  // bypass (fm without filter = discriminator output)
	
	// 				AF_UNKNOWN4  = 10u,  // nothing at all
	// 				AF_UNKNOWN5  = 11u,  // ? distorted
	// 				AF_UNKNOWN6  = 12u,  // ? distorted
	// 				AF_UNKNOWN7  = 13u,  // ? interesting
	// 				AF_UNKNOWN8  = 14u,  // ? interesting 
	// 				AF_UNKNOWN9  = 15u   // not a lot	
	
	// <7:1>		?				
	// <0>		0	AF TX filter bypass all:
					// 1: Bypass all AF TX filter
					// 0: Normal	

    BK4819_Write_Register(0x47, 0x6040 | (af << 8));			// Scrive nel registro 47 per configurare la modalità AF
}

// ---------------------------------------------------- Cancella Interrupt con timeout
void BK4819::BK4819_Clear_Interrupt(void)
{
    uint16_t timeout_counter = 100;  							// Definisci un numero massimo di tentativi

    while (timeout_counter--) 									// Diminuisci il contatore a ogni iterazione
    {
        BK4819_Write_Register(0x02, 0); 						// Cancella gli interrupt pendenti

        const uint16_t Status = BK4819_Read_Register(0x0C); 	// Registro Interrupt
        if ((Status & 1u) == 0) break; 							// Se non ci sono più interrupt, esci dal ciclo

        delayMicroseconds(100); // tempo di attesa per la lettura successiva
    }
}




// ---------------------------------------------------- Interrupt set
void BK4819::BK4819_IRQ_Set (BK4819_IRQType_t InterruptMask)
{
	BK4819_Clear_Interrupt();
	BK4819_Write_Register(0x3F, 0 | InterruptMask); 			// Registro Mascheramento interrupt
}	

// ---------------------------------------------------- Check Stato Interrupt
BK4819_IRQType_t BK4819::BK4819_Check_Irq_type( void )
{
	BK4819_Write_Register(0x02, 0);								// Cancella gli interrupt pendenti
	const BK4819_IRQType_t irq = BK4819_Read_Register(0x02);	// legge gli interrupt in atto
	return irq;
}	

// ---------------------------------------------------- Imposta Frequenza
void BK4819::BK4819_Set_Frequency(uint32_t Frequency)
{
	const uint32_t Freq = Frequency/10;

	BK4819_Write_Register(0x38, (Freq >>  0) & 0xFFFF);
	BK4819_Write_Register(0x39, (Freq >> 16) & 0xFFFF);

	BK4819_Write_Register(0x30, 0);
	BK4819_Write_Register(0x30, 0b1011111111110001); 
}

// ---------------------------------------------------- Set registri AGC
void BK4819::BK4819_RF_Set_Agc(u8 mode)
{
	switch(mode)
	{
		case 0:
			BK4819_Write_Register(0x13,0x03BE);
			BK4819_Write_Register(0x12,0x037B);
			BK4819_Write_Register(0x11,0x027B);
			BK4819_Write_Register(0x10,0x007A);
			BK4819_Write_Register(0x14,0x0019);
			
			BK4819_Write_Register(0x49,0x2A38);
			BK4819_Write_Register(0x7B,0x8420);			// <15:0>	RSSI Table
			BK4819_Write_Register(0x7C,0x8000);			// <15:0>	RSSI Table
			break;
		
		case 1:
			BK4819_Write_Register(0x13,0x03BE);
			BK4819_Write_Register(0x12,0x0B7C);
			BK4819_Write_Register(0x11,0x035B);
			BK4819_Write_Register(0x10,0x031A);
			BK4819_Write_Register(0x14,0x0258);
			
			BK4819_Write_Register(0x49,0x2A38);
			BK4819_Write_Register(0x7B,0x318C);			// <15:0>	RSSI Table
			BK4819_Write_Register(0x7C,0x595E);			// <15:0>	RSSI Table
			BK4819_Write_Register(0x20,0x8DEF); 
			//for(int i=0;i<8;i++) BK4819_Write_Register(0x06,(u16) ((i&7)<<13 | 0x0E<<7 | 0x41));
			break;
	}                                                                                                                                                                                                                                                                                                 
}

// ---------------------------------------------------- Imposta Gain e livello
void  BK4819::BK4819_Set_AGC_Gain(uint8_t Agc, uint8_t Value)
{

    // UART_printf("reg = %04x, %d \n",Reg_Value, fix);
	//
	// <15> 0 AGC Fix Mode.		 							1=Fix; 0=Auto.
	//														  3				2   1   0  7   6   5   4
	// <14:12> 0b011 AGC Fix Index.   						011=Max, then 010,001,000,111,110,101,100(min).
	// 														default fix index too strong, set to min (011->100)
	//
	// <5:3> 0b101 DC Filter Band Width for Tx (MIC In).  	000=Bypass DC filter;
	//
	// <2:0> 0b110 DC Filter Band Width for Rx (IF In).		000=Bypass DC filter;
	
	const uint8_t gAGCfix = (Agc>AGC_AUTO)?1:0;
	const uint8_t gAGCidx = 3;
	
	//		<15>	AGC fix mode:  1= Fix  0= Auto	
	//	 <14:12>	AGC fix index: 3,2,1,0,4
	//     <5:3>	DC filter bandwidth for TX (MIC in): 000: Bypass DC filter
	//     <2:0>	DC filter bandwidth for RX (IF in) : 000: Bypass DC filter
	
	const uint16_t regVal = BK4819_Read_Register(0x7E);
	BK4819_Write_Register(0x7E, (regVal & ~(1 << 15) & ~(0b111 << 12)) |
								(gAGCfix << 15) | 
								(gAGCidx << 12) | 
								(5u << 3)       | 
								(6u << 0));


	// REG_13
	//
	// 0x0038 Rx AGC Gain Table[0]. (Index Max->Min is 3,2,1,0,-1)
	//
	// <15:10> ???
	//
	// <9:8>   LNA Gain Short
	//         3 =   0dB  <<<
	//         2 = -24dB       // was -11
	//         1 = -30dB       // was -16
	//         0 = -33dB       // was -19
	//
	// <7:5>   LNA Gain
	//         7 =   0dB
	//         6 =  -2dB
	//         5 =  -4dB <<<
	//         4 =  -6dB
	//         3 =  -9dB
	//         2 = -14dB 
	//         1 = -19dB
	//         0 = -24dB
	//
	// <4:3>   MIXER Gain
	//         3 =   0dB <<<
	//         2 =  -3dB
	//         1 =  -6dB
	//         0 =  -8dB
	//
	// <2:0>   PGA Gain
	//         7 =   0dB
	//         6 =  -3dB <<<
	//         5 =  -6dB
	//         4 =  -9dB
	//         3 = -15dB
	//         2 = -21dB
	//         1 = -27dB
	//         0 = -33dB
	//
	
	BK4819_Write_Register(0x12, (3u << 8) | (3u << 5) | (3u << 3) | (4u << 0));  // 000000 11 011 11 100  0x037C =  3 3 3 4
	BK4819_Write_Register(0x11, (2u << 8) | (3u << 5) | (3u << 3) | (3u << 0));  // 000000 10 011 11 011  0x027B =  2 3 3 3
	BK4819_Write_Register(0x10, (0u << 8) | (3u << 5) | (3u << 3) | (2u << 0));  // 000000 00 011 11 010  0x007A =  0 3 3 2
	BK4819_Write_Register(0x14, (0u << 8) | (0u << 5) | (3u << 3) | (1u << 0));  // 000000 00 000 11 000  0x0019 =  0 0 3 1	

	if(gAGCfix)
	{
		BK4819_Write_Register(0x13, gain_table[Value].reg_val );
		BK4819_Write_Register(0x49, (0 << 14) | (50 << 7) | (15 << 0)); 
	}
	else
	{
		BK4819_Write_Register(0x13, 0x295);
		BK4819_Write_Register(0x49, (0 << 14) | (84 << 7) | (56 << 0));
	}

	// BK4819_Write_Register(BK4819_REG_49, 0x2A38);
	//														<15:14> 00 = auto / 10 = Low Lo / 11 = High Lo
	//														<13:7>  High Threshold LSB 1dB
	//														<6:0>   Low Threshold LSB  1dB
	// 54 3210987 6543210
	// 00 1010100 0111000 		84 56						slow 45 25 / fast 50 15


	// TBR: listed as two values, agc_rssi and lna_peak_rssi
	// This is why AGC appeared to do nothing as-is for Rx
	//
	// REG_62
	//
	// <15:8> 0xFF AGC RSSI
	//
	// <7:0> 0xFF LNA Peak RSSI
	//
	// TBR: Using S9+30 (173) and S9 (143) as suggested values
	// BK4819_Write_Register(BK4819_REG_62, (250u << 8) | (150u << 0));

}

// ---------------------------------------------------- Imposta correzioni frequenza clock 
void BK4819::BK4819_Set_Xtal(BK4819_Xtal_t mode)
{
	switch(mode)
	{
		case XTAL26M:
			break;
			
		case XTAL13M:
			// BK4819_Write_Register(0x40,REG_40 | DEVIATION); //DEVIATION=0x5D0 for example
			BK4819_Write_Register(0x41,0x81C1);                    
    		BK4819_Write_Register(0x3B,0xAC40);
    		BK4819_Write_Register(0x3C,0x2708); 
    		BK4819_Write_Register(0x3D,0x3555);		// registro IF
			break;
			
		case XTAL19M2:
			// BK4819_Write_Register(0x40,REG_40 | DEVIATION); //DEVIATION=0x53A for example
			BK4819_Write_Register(0x41,0x81C2);                    
    		BK4819_Write_Register(0x3B,0x9800);
    		BK4819_Write_Register(0x3C,0x3A48); 
    		BK4819_Write_Register(0x3D,0x2E39);		// registro IF
			break;
			
		case XTAL12M8:
			// BK4819_Write_Register(0x40,REG_40 | DEVIATION); //DEVIATION=0x5D0 for example
			BK4819_Write_Register(0x41,0x81C1);                    
    		BK4819_Write_Register(0x3B,0x1000);
    		BK4819_Write_Register(0x3C,0x2708); 
    		BK4819_Write_Register(0x3D,0x3555);		// registro IF
			break;
			
		case XTAL25M6:
    		BK4819_Write_Register(0x3B,0x2000);
    		BK4819_Write_Register(0x3C,0x4E88); 
			break;
			
		case XTAL38M4:
			// BK4819_Write_Register(0x40,REG_40 | DEVIATION); //DEVIATION=0x43A for example
			BK4819_Write_Register(0x41,0x81C5);                    
    		BK4819_Write_Register(0x3B,0x3000);
    		BK4819_Write_Register(0x3C,0x75C8); 
    		BK4819_Write_Register(0x3D,0x261C);		// registro IF
			break;
	}
}

// ---------------------------------------------------- Set Automatic Frequency control
void BK4819::BK4819_Set_AFC(uint8_t value)
{
	// -----------------misurati con Wide standard
	// valore 0 = 26 K
	// valore 3 = 15 K 
	// valore 5 = 12.6K 
	
	switch(value)
	{ 
		case 0:
			BK4819_Write_Register(0x73, 0u  |
								 (7 <<  11) |     				//  0 AFC Max / 111 Min
								 (1 <<  4));      				//  1 disable / 0 enable
            break; 															
	
		default:
			BK4819_Write_Register(0x73, 0u  |
							     (((8-value) & 7) <<  11) |     //  0 AFC Max / 111 Min
							     (0             <<  4));      	//  1 disable / 0 enable
			break;
	}
}

// ---------------------------------------------------- Set Squelch
void BK4819::BK4819_Set_Squelch
(
	uint8_t Squelch_Open_RSSI,
	uint8_t Squelch_Close_RSSI,
	uint8_t Squelch_Open_Noise,
	uint8_t Squelch_Close_Noise,
	uint8_t Squelch_Close_Glitch,
	uint8_t Squelch_Open_Glitch )
{
	
	// REG_70
	//
	// <15>   0 Enable TONE1
	//        1 = Enable
	//        0 = Disable
	//
	// <14:8> 0 TONE1 tuning gain
	//        0 ~ 127
	//
	// <7>    0 Enable TONE2
	//        1 = Enable
	//        0 = Disable
	//
	// <6:0>  0 TONE2/FSK tuning gain
	//        0 ~ 127
	//
	BK4819_Write_Register(0x70, 0);

	// Glitch threshold for Squelch = close
	//
	// 0 ~ 255
	//
	BK4819_Write_Register(0x4D, 0xA000 | Squelch_Close_Glitch);		

	// REG_4E
	//
	// <15:14> 1 ???
	//
	// <13:11> 5 Squelch = open  Delay Setting
	//         0 ~ 7
	//
	// <10:9>  7 Squelch = close Delay Setting
	//         0 ~ 3
	//
	// <8>     0 ???
	//
	// <7:0>   8 Glitch threshold for Squelch = open
	//         0 ~ 255
	//
	BK4819_Write_Register(0x4E,  			// 01 101 11 1 00000000  // original (*)
						 (1u << 14) |       //  1 ???
						 (5U << 11) |       // *5  squelch = open  delay .. 0 ~ 7		
		                 (6u <<  9) |       // *3  squelch = close delay .. 0 ~ 7		
		                  Squelch_Open_Glitch);  //  0 ~ 255

	// REG_4F
	//
	// <14:8> 47 Ex-noise threshold for Squelch = close
	//        0 ~ 127
	//
	// <7>    ???
	//
	// <6:0>  46 Ex-noise threshold for Squelch = open
	//        0 ~ 127
	//
	BK4819_Write_Register(0x4F, ((uint16_t)Squelch_Close_Noise << 8) | Squelch_Open_Noise);

	// REG_78
	//
	// <15:8> 72 RSSI threshold for Squelch = open    0.5dB/step
	//
	// <7:0>  70 RSSI threshold for Squelch = close   0.5dB/step
	//
	BK4819_Write_Register(0x78, ((uint16_t)Squelch_Open_RSSI   << 8) | Squelch_Close_RSSI);

}

// ---------------------------------------------------- Squelch mode set
void BK4819::BK4819_Squelch_Mode ( BK4819_SquelchMode_t mode )
{
	// 0x88/0xA8: RSSI + noise + Glitch
	// 0xaa: 	  RSSI + Glitch
	// 0xcc: 	  RSSI + noise
	// 0xFF:      RSSI
	
	BK4819_Write_Register(0x77, 0 | (mode << 8) | 0xFF);	
}	


// ---------------------------------------------------- Leggi RSSI ritorna in dbM
int16_t BK4819::BK4819_Get_RSSI (void)
{
	uint16_t reg = BK4819_Read_Register(0x67);
	int16_t rssi = (reg & 0x01FF)/2-160;
	return rssi;  
}	


// ---------------------------------------------------- Modo Sleep
void BK4819::BK4819_Sleep(void)
{
	BK4819_Write_Register(0x30, 0);
	
	// DSP Voltage Setting = 1			1 
	
	// ANA LDO = 2.7v					1
	// VCO LDO = 2.7v					1
	// RF LDO  = 2.7v					0
	// PLL LDO = 2.7v					1
	
	// ANA LDO bypass					0
	// VCO LDO bypass					0
	// RF LDO  bypass					0
	// PLL LDO bypass					0
	
	// Reserved bit is 1 instead of 0	0
	// Enable  DSP						0
	// Enable  XTAL						0
	// Enable  Band Gap					0
	//
	BK4819_Write_Register(0x37, 0x1D00);
}

// ---------------------------------------------------- disabilita ricezione DTMF
void BK4819::BK4819_Disable_DTMF(void)
{
	// REG_24
	// --------------------------------
	// <5>		0		DTMF/SelCall enable:
										// 1: Enable
										// 0: Disable
										
	// <4>		1		DTMF or SelCall detection mode:
										// 1: DTMF
										// 0: SelCall
										
    // <3:0>	0xe		Max. symbol number for SelCall detection
   
	BK4819_Write_Register(0x24, 0);
	
	uint16_t InterruptMask = BK4819_Read_Register(0x3F);		// Legge registro interrupt
	InterruptMask &= ~BK4819_REG_3F_DTMF_5TONE_FOUND;			// disabilita eventuale interrupt ricezione
	
	BK4819_Write_Register(0x3F, InterruptMask);					// Registro Mascheramento interrupt
}

// ******************************************************************************************************************************
//																											 SEZIONE TRASMISSIONE
// ******************************************************************************************************************************

// ---------------------------------------------------- Abilita audio microfono
void BK4819::BK4819_Enable_Mic ( uint8_t MIC_SENSITIVITY_TUNING )
{
	BK4819_Disable_DTMF();						// se la DTMF e' abilitata in trasmissione altera la modulazione restringendo la BW audio
	
	BK4819_Write_Register(0x7D, 0xE940 | (MIC_SENSITIVITY_TUNING & 0x1f));	// MIC sensitivity tuning, 0.5 dB/step
																			// 0 	= min
																			// 0x1F = max
																
	BK4819_Write_Register(0x19, 0x1041);		// Automatic MIC PGA Gain Controller 
												//   5432109876543210
/* 			 			 0		   |			// 0b0001000001000001);     
						(0u << 15) |     		// <15> MIC AGC  1 = disable  0 = enable
						(1u	<< 12) |			// ?
						(1u	<< 6 ) |			// ?
						(1u << 0 ));			// ? 
					
					// 5432 1098 7654 3210			
					// 0
					//    1 
					// 			  1	
					// 	                 1
					// ___________________
					// 0001 0000 0100 0001
*/										
										
}

// ---------------------------------------------------- Abilita Finale alla trasmissione		
void BK4819::BK4819_Set_Power_Amplifier(const uint8_t bias, uint8_t gain1, uint8_t gain2, bool enable)
{
	// REG_36 <15:8> 0 PA Bias output 0 ~ 3.2V
	//               255 = 3.2V
	//                 0 = 0V
	//
	// REG_36 <7>    0
	//               1 = Enable PA-CTL output
	//               0 = Disable (Output 0 V)
	//
	// REG_36 <5:3>  7 PA gain 1 tuning
	//               7 = max
	//               0 = min
	//
	// REG_36 <2:0>  7 PA gain 2 tuning
	//               7 = max
	//               0 = min
	//
	//                                  280MHz       gain 1 = 1  gain 2 = 0  gain 1 = 4  gain 2 = 2
	// const uint8_t gain = (frequency < 28000000) ? (1u << 3) | (0u << 0) : (4u << 3) | (2u << 0);
	//const uint8_t gain = (frequency < 28000000) ? (4u << 3) | (4u << 0) : (4u << 3) | (2u << 0);

	BK4819_Write_Register(0x36, (bias << 8) | (enable << 7) | (gain1 << 3) | (gain2 << 0));
}

// ---------------------------------------------------- 
void BK4819::BK4819_Enable_TXLink(void)
{
	BK4819_Write_Register(0x30, 0 |
			BK4819_REG_30_ENABLE_VCO_CALIB |	// <15>
			BK4819_REG_30_ENABLE_UNKNOWN   |	// <14>
			BK4819_REG_30_DISABLE_RX_LINK  |	// <13:10>
			BK4819_REG_30_ENABLE_AF_DAC    |	// <9>
			BK4819_REG_30_ENABLE_DISC_MODE |	// <8>
			BK4819_REG_30_ENABLE_PLL_VCO   |	// <7:4>
			BK4819_REG_30_ENABLE_PA_GAIN   |	// <3>
			BK4819_REG_30_DISABLE_MIC_ADC  |	// <2>
			BK4819_REG_30_ENABLE_TX_DSP    |	// <1>
			BK4819_REG_30_DISABLE_RX_DSP);		// <0>
	
}

// ---------------------------------------------------- 
void BK4819::BK4819_Disable_TXLink(void)
{
	BK4819_Write_Register(0x30, 0xC1FE);   // 1 1 0000 0 1 1111 1 1 1 0	

/* 	BK4819_Write_Register(0x30, 0  		   |
			BK4819_REG_30_ENABLE_VCO_CALIB |	// <15>
//			BK4819_REG_30_ENABLE_UNKNOWN   |    // <14>
			BK4819_REG_30_ENABLE_RX_LINK   |    // <13:10>
			BK4819_REG_30_ENABLE_AF_DAC    |    // <9>
			BK4819_REG_30_ENABLE_DISC_MODE |    // <8>
			BK4819_REG_30_ENABLE_PLL_VCO   |    // <7:4>
//			BK4819_REG_30_ENABLE_PA_GAIN   |    // <3>
//			BK4819_REG_30_ENABLE_MIC_ADC   |    // <2>
//			BK4819_REG_30_ENABLE_TX_DSP    |    // <1>
			BK4819_REG_30_ENABLE_RX_DSP); */    // <0>
}

// ---------------------------------------------------- Deviazione trasmissione
void BK4819::BK4819_Set_TxDeviation ( uint16_t value )
{
	// REG_40   RF TxDeviation.
	// <13>		1   abilita i toni DCS ( non documentato se messo a 0 non funzionano piu' )
	// <12>		1	Enable RF TxDeviation.  1=Enable; 0=Disable
	//
	// <11:0>	4D0	RF Tx Deviation Tuning (Apply for both in-band signal and sub-audio signal). 0=min; 0xFFF=max 
	
	if (value == 0)	BK4819_Write_Register(0x40,(2u << 12) | (value & 0xFFF));	// con zero disabilita la deviazione in trasmissione
	else 			BK4819_Write_Register(0x40,(3u << 12) | (value & 0xFFF));	// con qualsiasi altro valore la abilita.
}

// ---------------------------------------------------- Mute Audio Tx per invio toni o altro
void BK4819::BK4819_Mute_Tx(bool mute)
{

	// REG_50
	// 			<15>	0		Enable AF TX mute (for DTMF TX or other applications):
	// 																					1: Mute
	// 																					0: Normal

	if(mute) BK4819_Write_Register(0x50, 0xBB20);	// mute TX
	else 	 BK4819_Write_Register(0x50, 0x3B20);	// Un-mute TX	
}

// ---------------------------------------------------- 
/*
void BK4819_Exit_Bypass(void)
{
	BK4819_Set_AF(AF_MUTE);							// disabilita ricezione

	// REG_7E
	//
	// <15>    0 AGC fix mode
	//         1 = fix
	//         0 = auto
	//
	// <14:12> 3 AGC fix index
	//         3 ( 3) = max
	//         2 ( 2)
	//         1 ( 1)
	//         0 ( 0)
	//         7 (-1)
	//         6 (-2)
	//         5 (-3)
	//         4 (-4) = min
	//
	// <11:6>  0 ???
	//
	// <5:3>   5 DC filter band width for Tx (MIC In)
	//         0 ~ 7
	//         0 = bypass DC filter
	//
	// <2:0>   6 DC filter band width for Rx (I.F In)
	//         0 ~ 7
	//         0 = bypass DC filter
	//
	
	BK4819_Write_Register(0x7E, (gAGCfix << 15) | (gAGCidx << 12) | (5u << 3) | (6u << 0));
		// (0u << 15) |      // 0  AGC fix mode
		// (3u << 12) |      // 3  AGC fix index
		// (5u <<  3) |      // 5  DC Filter band width for Tx (MIC In)
		// (6u <<  0));      // 6  DC Filter band width for Rx (I.F In) 
	
	
	// BK4819_Write_Register(BK4819_REG_7E, // 0x302E);   // 0 011 000000 101 110
		// (0u << 15) |      // 0  AGC fix mode
		// (3u << 12) |      // 3  AGC fix index
		// (5u <<  3) |      // 5  DC Filter band width for Tx (MIC In)
		// (6u <<  0));      // 6  DC Filter band width for Rx (I.F In) 
}
*/

// ---------------------------------------------------- 
void BK4819::BK4819_Prepare_Transmit(void)
{
	// BK4819_Exit_Bypass();
	BK4819_Set_Power_Amplifier(0, 2, 2, 1);
	BK4819_Mute_Tx(false);
	BK4819_TxOn();
}

// ---------------------------------------------------- 
void BK4819::BK4819_TxOn(void)
{
	// Register 37
	// DSP Voltage Setting = 1			1 
	
	// ANA LDO = 2.7v					1
	// VCO LDO = 2.7v					1
	// RF LDO  = 2.4v					0
	// PLL LDO = 2.7v					1
	
	// ANA LDO bypass					0
	// VCO LDO bypass					0
	// RF LDO  bypass					0
	// PLL LDO bypass					0
	
	// Reserved bit is 1 instead of 0	1
	// Enable  DSP						1
	// Enable  XTAL						1
	// Enable  Band Gap					1
	//
	BK4819_Write_Register(0x37, 0x1D0F);
	BK4819_Write_Register(0x30, 0x0000);	// Turn OFF
	BK4819_Write_Register(0x30, 0xC1FE);	// Turn ON TX
}
