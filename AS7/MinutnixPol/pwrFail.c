/*
 * pwrFail.c
 *
 * Created: 3/14/2019 11:11:01 PM
 *  Author: stefan
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

typedef struct
{
	int Dane;
	int Temperatura;
	int PID_P;
} DaneStrona;

EEMEM DaneStrona DaneEEPROM; // = {.Dane=0xaabb, .Temperatura=0xac, .PID_P=0x10}; Dane w EEPROM

DaneStrona DaneSRAM; // Kopia danych w SRAM

ISR(ANALOG_COMP_vect, ISR_NAKED)
{
	// Wylacz zbedne generatory i zmniejsz FCLK
	// Dodatkowo warto wszystkie piny ustawiæ jako wejscia
	// Rozpoczynamy zapis zmiennych do EEPROM

	eeprom_update_block(&DaneSRAM, &DaneEEPROM, sizeof(DaneStrona)); // Skopiuj dane do EEPROM
	
	while(1); // Juz nic wiecej nie robimy
}

void AC_init()
{
	DIDR1 =_BV(AIN1D) | _BV(AIN0D);   // Wylacz porty cyfrowe zwiazane z wejsciami komparatora
	ACSR =_BV(ACIE) | _BV(ACIS1);   // Wlacz komparator i przerwanie komparatora zwiazane ze zboczem opadajacym
}

void Data_init()
{
	eeprom_read_block(&DaneSRAM, &DaneEEPROM, sizeof(DaneStrona));  // Skopiuj dane z EEPROM do SRAM
}

/*
int main(void)
{
	DDRB |=_BV(PB5);
	PORTB |= _BV(PB5);

	Data_init();
	AC_init();
	sei(); // Odblokuj przerwania
	
	DaneSRAM.Dane=0x4BCD;  // Modyfikujemy odtworzona kopie danych
	DaneSRAM.PID_P=0x0211; // Zostana one automatycznie zapisane po odlaczeniu zasilania
	DaneSRAM.Temperatura=0x1234;

	while(1)
	{
		// IT WORKED!
	}
}
*/