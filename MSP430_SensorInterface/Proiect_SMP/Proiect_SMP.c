#include <msp430.h>
#include "LiquidCrystal_I2C.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>


void DelayMs(unsigned int Ms);
#define MCLK_FREQ_MHZ 16                                                // MCLK = xMHz

typedef enum {
    PARAM_OK,
    VENT_ON
} SystemState;
SystemState systemState;
char* stateToString(SystemState state);

char welcomeMessage[100];


void updateMessage(int temperature, int counter, SystemState state);
void Reset_LCD();


void Configure_UART();
void Init_ClockSystem();
void Configure_TimerB();
void Configure_ADC();
void Configure_WDT();

volatile unsigned int Temperatura;
volatile unsigned int Temperature_display;
volatile unsigned int counter = 0;
volatile unsigned int counter_display;
volatile float v;
float Rt, tempK, tempC;
_Bool reset = false;
int UART_TX_buff_index = 0;
int i=0;

char UART_TX_buff_message[50];

void main(void)
{

   counter_display = Temperature_display;

   Configure_WDT();


   /*    P2.3 input buton ---> Increase threshold  */
   P2OUT |= BIT3;                          /* Configure P2.3 as pulled-up */
   P2REN |= BIT3;                          /* P2.3 pull-up register enable */
   P2IES |= BIT3;                          /* P2.3 Hi/Low edge */
   P2IE |= BIT3;                           /* P2.3 interrupt enabled */
   /*    P4.1 input buton ---> Descrease threshold             */
   P4OUT |= BIT1;                          /* Configure P4.1 as pulled-up */
   P4REN |= BIT1;                          /* P4.1 pull-up register enable */
   P4IES |= BIT1;                          /* P4.1 Hi/Low edge */
   P4IE |= BIT1;                           /* P4.1 interrupt enabled */
   /*    P6.6 ---> WDT signal light   */
   P6DIR |= BIT6;
   P6OUT &=~BIT6;
   /*    P1.0 --> ADC signal light     */
   P1DIR |= BIT0;
   P1OUT &=~BIT0;
   /*    Configure ADC A1 pin P1.1   */
   P1SEL0 |= BIT1;
   P1SEL1 |= BIT1;
   /*    P2.0 --> Buzzer Pin      */
   P2DIR |= BIT0;
   P2SEL1 &=~ BIT0;
   P2SEL0 |= BIT0;
   /*    I2C pins    */
   P2DIR |= BIT1 | BIT2; /* Configure P2.1 and P2.2 as output */

   Configure_TimerB();

   Configure_ADC();

   Init_ClockSystem();

   Configure_UART();

   // Configure reference
   PMMCTL0_H = PMMPW_H;                                          // Unlock the PMM registers
   PMMCTL2 |= REFVSEL_0 | INTREFEN | TSENSOREN;  // Enable internal reference and temperature sensor
   __delay_cycles(400);                                          // Delay for reference settling


   PMMCTL0_H = PMMPW_H;                    // Unlock the PMM registers
   PMMCTL2 = INTREFEN | REFVSEL_0;        // Enable internal 1.5V reference
   while(!(PMMCTL2 & REFGENRDY));          // Poll till internal reference settles

   PM5CTL0 &= ~LOCKLPM5;       // Disable high-impedance mode.

    __enable_interrupt();                        //Enable maskable interrupts
    SFRIE1 |= WDTIE;        // Enable WDTIE

    // LCD function call and setups:
    I2C_Init(0x3F);             // 0x27 signifies the slave address (address of the LCD with the I/O expander).
    LCD_Setup();
    LCD_SetCursor(0, 0);        // Initial position for the cursor at row 1, column 1.

    while (1)
    {

        v = Temperatura * 3.3 / 1024 ;

        Rt = 10 * v / (3.3 - v);
        // Calculate temperature (Kelvin)
        tempK = 1 / (log(Rt / 10) / 3950 + 1 / (273.15 + 27));
        // Calculate temperature (Celsius)
        tempC = tempK - 273.15;
        Temperature_display = (int)tempC;

        LCD_ClearDisplay();

        LCD_SetCursor(0, 0);
        LCD_Write("Temp:");
        LCD_WriteNum(Temperature_display);

        LCD_SetCursor(8,0);
        LCD_Write("@");


        LCD_SetCursor(0, 1);
        LCD_Write("Set: ");
        LCD_WriteNum(counter_display);

        LCD_SetCursor(8,1);
        LCD_Write(stateToString(systemState));
        Reset_LCD();

        if (Temperature_display <= counter_display)
        {
            systemState = PARAM_OK;
            P2OUT |= BIT1; // Turn on LED at P2.1
            P2OUT &= ~BIT2; // Turn off LED at P2.2
            TB1CCR1 = 0;

        }
        else if (Temperature_display > counter_display)
        {
            systemState = VENT_ON;
            P2OUT |= BIT2; // Turn on LED at P2.2
            P2OUT |= BIT0;
            P2OUT &= ~BIT1; // Turn off LED at P2.1
            TB1CCR1 = 1000;

        }
        else
        {
            systemState = PARAM_OK;
            P2OUT &= ~(BIT0 | BIT1 | BIT2); // Turn off both LEDs if the temperature is equal to the counter_display
            TB1CCR1 = 0;

        }

        updateMessage(Temperature_display, counter_display, systemState);

        DelayMs(250);
    }
}


/****************************************_Config_WDT_***************************************************/
void Configure_WDT()
{
   /* SMCLK --> t_clk = 1/f = 1/ 1.000.000  = 1 * 10^-6 = 1 ms
   // Qx = t_int / t_clk  => t_int = Qx * t_clk => 8.38 = Qx * t_clk => Qx = 8.38 / t_clk => Qx = 2 / 1* 10^-6 = 8.388 * 10^6 ~ 2^23 => Qx = 2^23 => WDTIS_2 */

   WDTCTL = WDTPW | WDTHOLD_0 | WDTSSEL__SMCLK | WDTTMSEL_1 | WDTCNTCL_1 | WDTIS_2;  /* WDT not STOP; SMCLK ; Interval ; ... ; WDT CLK Source */
+
}

/*****************************************_Reset_LCD_***************************************************/
void Reset_LCD()
{
    if (reset == true)
    {
       I2C_Init(0x3F);
       LCD_ClearDisplay();
       reset = false;

    }

}
/*****************************************_DelayMs_*****************************************************/
void DelayMs(unsigned int Ms)
{
    while(Ms)
    {
        __delay_cycles(1000);
        Ms--;
    }
}


/**************************************_ADC_VECTOR_ISR_*************************************************/
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
    volatile float temp;
    volatile float IntDegF;
    volatile float IntDegC;

    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {

        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:

           Temperatura = ADCMEM0;


            break;
        default:
            break;
    }
}

/**************************************_TIMER0_B0_VECTOR_ISR_*********************************************/
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer_B (void)
{
    ADCCTL0 |= ADCENC | ADCSC; /* Sampling and conversion start */
    P1OUT ^= BIT0;

    for( UART_TX_buff_index = 0 ;  UART_TX_buff_index <=sizeof(UART_TX_buff_message);  UART_TX_buff_index++)
    {
        while(!(UCA1IFG&UCTXIFG));
        UCA1TXBUF = UART_TX_buff_message[UART_TX_buff_index];
    }

}



/******************************************_PORT2_VECTOR_ISR_*********************************************/
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
    if (counter_display < 30) {
        ++counter_display;
    } else {
        counter_display = 30;
    }
    P2IFG &= ~BIT3;                         /* Clear P2.3 IFG */
    DelayMs(50);                            /* Add delay for debouncing */
    __bic_SR_register_on_exit(LPM3_bits);   /* Exit LPM3 */
}

/******************************************_PORT4_VECTOR_ISR_*********************************************/
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
    if (counter_display > 15)
    {
        --counter_display;
    }
    else
    {
        counter_display = 30;
    }
    P4IFG &= ~BIT1;                         /* Clear P4.1 IFG */
    DelayMs(50);                            /* Add delay for debouncing */
    __bic_SR_register_on_exit(LPM3_bits);   /* Exit LPM3 */
}


/*******************************************_WDT_VECTOR_ISR_*********************************************/
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{
   reset = true;
   P6OUT ^= BIT6;
   DelayMs(100);
   P6OUT ^= BIT6;
}


/*******************************************_UART_1_ISR_*************************************************/
#pragma vector=USCI_A1_VECTOR                                          /* UART 1 ISR   (USCI_A1_VECTOR) */
__interrupt void USCI_A1_ISR(void)
{
  switch(__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
            if(UCA1RXBUF == 60)
            {
                 --counter_display;
            }
            else if (UCA1RXBUF == 62)
            {
                 ++counter_display;
            }
          __no_operation();
          break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
  }
}




/*********************************************_Configure_UART_*******************************************/
void Configure_UART()                                                   /* Functie care configureaza UART            */
{
  /*                                         Configure_UART_pins                                                     */
  P1SEL0 |= BIT6 | BIT7;                                                /* set 2-UART pin as second function         */
  /*                    P4.3 -> TxD   P4.2 -> RxD  pentru vizualizare pe osciloscop                                  */
  P4SEL0 = BIT2 | BIT3;                                                 /* selectam functiile  TxD si RxD            */

  /*                                           Configure_UART                                                        */
  UCA1CTLW0 |= UCSWRST;                                                 /* Software reset enable                     */
  UCA1CTLW0 |= UCSSEL_2;                                                /* set SMCLK as BRCLK = 1MHz                 */

  /*                            Set_Baud_Rate_to_19200_bps_for_BRCLK_=_SMCLK_=1Mhz                                   */
  UCA1BR0 = 0x03;                                                       /* INT(1000000/19200)                        */
  UCA1BR1 = 0x00;                                                       /* USCI A1 Baud Rate 1                       */
  /*                    First 2bytes: UCBRSx = 0x00; 3rd & 4th: UCBRFx = 4(3rd) & OS16 = 1(4th)                      */
  UCA1MCTLW =0x0041;                                                    /* eUSCI_Ax Modulation Control Word Register */

  UCA1CTLW0 &= ~UCSWRST;                                                /* Initialize eUSCI                          */
  UCA1IE |= UCRXIE;                                                     /* Enable USCI_A0 RX interrupt               */

}


/*******************************************_Init_ClockSystem_*******************************************/
void Init_ClockSystem()                                                 /* Functie care initializeaza Clock System CS  */
{
    FRCTL0 = FRCTLPW | NWAITS_2;                                        /* FRAM Controller Control Register 0 */

    __bis_SR_register(SCG0);                                            /* Disable FLL */
    CSCTL3 |= SELREF__REFOCLK | FLLREFDIV_0;                            /* Alegem frecventa sa fie luata de la REFOCLK si FLLREFDIV = 0 */
    CSCTL0 = 0;                                                         /* facem clean la registri DCO si MOD */
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_5;          /* DCOFTRIMEN: Freq.Trim ; ... ; ... ; DCORSEL: Select MCLK Freq */

    /*                        Calcul: --> f_mult = f_MCLK / F_REFOCLK = 16.000.000 Hz / 32768 Hz = 488 */
    CSCTL2 = FLLD_0 + 487;                                              /*configuram factorul de divizare si de multiplicare, FLLD = 1 si FLLN = f_mult - 1 */
    __delay_cycles(3);                                                  /*configuratii necesare */
    __bic_SR_register(SCG0);                                            /* Enable FLL */

    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;                          /* set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz */
                                                                        /* default DCODIV as MCLK and SMCLK source */
    CSCTL5 = DIVM__16  | DIVS_0;                                        /* MCLK = 16MHz si SMCLK = 1MHz */

}


/**********************************_Creating _msg_for_UARTTXBUF_*****************************************/
void updateMessage(int temperature, int counter, SystemState state)
{
    char stateStr[10];
    if (state == PARAM_OK) {
        snprintf(stateStr, sizeof(stateStr), "TempOK");
    } else if (state == VENT_ON) {
        snprintf(stateStr, sizeof(stateStr), "VentON");
    }

    snprintf(UART_TX_buff_message, sizeof(UART_TX_buff_message), "Temp :%d Set:%d SysSt:%s\r\n", temperature, counter, stateStr);

}

/*****************************************_State_to_string_*********************************************/
char* stateToString(SystemState state)
{
    switch(state)
    {
        case VENT_ON: return "VENT_ON";
        case PARAM_OK: return "PARAM_OK";
        default: return "UNKNOWN_STATE";
    }
}

/*******************************************_Init_TimerB_***********************************************/
void Configure_TimerB()
{
       /*                    TB0.0 --> get data from ADC                                               */
       TB0CCTL0 |= CCIE;                                             /* TBCCR0 interrupt enabled       */
       TB0CCR0 = 32768;                                              /* T = 1s                         */
       TB0CTL = TBSSEL__ACLK | MC__UP;                               /* ACLK, UP mode                  */

       /*                    TB1.1  --> PWM for Buzzer                                                 */
       TB1CCR1 = 2000;                                               /* Set buzzer duty cycle to 5%    */
       TB1CCR0 = 40000;                                              /* Set PWM period to 40000        */
       TB1CCTL1 = OUTMOD_7;                                          /* Reset/set mode for CCR1        */
       TB1CTL = TBSSEL_2 | ID_3 | TBIDEX__8 | MC_1 | TBCLR;                      /* ACLK, up mode, clear TBR       */

}

/*******************************************_Init_ADC_**************************************************/
void Configure_ADC()
{
  /*                Configure ADC - Pulse sample mode; ADCSC trigger                */
  ADCCTL0 |= ADCSHT_15 | ADCON;                                 /* ADC ON,temperature sample period>30us ; 1024 ADCCLK cycles */
  ADCCTL1 |= ADCSHP;                                            /* s/w trig, single ch/conv, MODOSC */
  ADCCTL2 |= ADCRES_1;                                          /* 10-bit conversion results */
  ADCMCTL0 |= ADCSREF_0 | ADCINCH_1;                            /* ADC input ch A1 => temp sense */
  ADCIE |=ADCIE0;                                               /* Enable the Interrupt request for a completed ADC_B conversion */

}
