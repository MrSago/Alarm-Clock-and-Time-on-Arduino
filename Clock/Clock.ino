/************************************************************

	Project name: Chasiki
	Author: MrS4g0

	Special for Nine Lashes

	27.05.2019

************************************************************/

#include <Wire.h>
#include <LiquidCrystal.h>
#include "./src/DS1302.h"

//-----------------------------------------------
// Global variables and constants

#define RTC_RST_PIN		13	// orange
#define RTC_DATA_PIN	12	// yellow
#define RTC_CLK_PIN		11	// green

#define BEEP_PIN	3


LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

DS1302 rtc(RTC_RST_PIN, RTC_DATA_PIN, RTC_CLK_PIN);

Time newTime, oldTime, alarmTime;

enum ButtonID
{
	BTN_RIGHT,
	BTN_UP,
	BTN_DOWN,
	BTN_LEFT,
	BTN_SELECT
};

int analogRange[7] = { -1, 100, 200, 400, 600, 800, 1024 };

uint8_t arrowLeft[] =
{
	B00000,
	B00100,
	B01000,
	B11111,
	B01000,
	B00100,
	B00000
};

uint8_t arrowRight[] =
{
	B00000,
	B00100,
	B00010,
	B11111,
	B00010,
	B00100,
	B00000
};

unsigned long int curIntBtn[5] = { 0 };
unsigned long int lastIntBtn[5] = { millis() };

struct ModeList
{
	void (*function)();
	ModeList* next;
} *CurrentMode = NULL;

enum Set
{
	SET_HOUR,
	SET_MIN,
	SET_SEC,

	SET_DATE,
	SET_MON,
	SET_YEAR
};

enum SetCoord
{
	CRD_HOUR = 4,
	CRD_MIN = 7,
	CRD_SEC = 10,

	CRD_DATE = 3,
	CRD_MON = 6,
	CRD_YEAR = 9
};

int selectMode = SET_HOUR;

bool setTime = false, setDate = false, setAlarm = false;
bool alarmOn = false, beepOn = false;

//-----------------------------------------------
// Functions declaration

void setup();
void loop();

void InitModeList();
void DefaultMode();
void SetTimeMode();
void SetDateMode();
void SetAlarmMode();

bool CheckButton(ButtonID id, unsigned int latency);
bool TimeEqual(Time t1, Time t2);
void LcdClearLine(uint8_t line);
void LcdPrintNum(int num);
void PrintArrows();
void DelArrows();
void SetCursorCrd();

//-----------------------------------------------
// Functions definition

//----------------------------------
//----------Setup function----------
//----------------------------------
void setup()
{
	Serial.begin(9600);

	pinMode(BEEP_PIN, OUTPUT);
	digitalWrite(BEEP_PIN, HIGH);

	lcd.begin(16, 2);
	lcd.display();
	lcd.noCursor();
	lcd.noBlink();
	lcd.setCursor(0, 0);
	lcd.createChar(0, arrowLeft);
	lcd.createChar(1, arrowRight);

	InitModeList();
	newTime = rtc.getTime();
}

//----------------

//----------------------------------
//--------Main loop function--------
//----------------------------------
void loop()
{
	if (!beepOn)
	{
		if (CheckButton(BTN_SELECT, 500))
		{
			CurrentMode = CurrentMode->next;

			if (setTime && !setAlarm)
			{
				if (!TimeEqual(newTime, oldTime))
				{
					rtc.halt(false);
					rtc.writeProtect(false);
					rtc.setTime(newTime.hour, newTime.min, newTime.sec);
					rtc.writeProtect(true);
					rtc.halt(true);
				}

				DelArrows();
				lcd.setCursor(0, 0);
				lcd.print("  ");
				lcd.setCursor(14, 0);
				lcd.print("  ");
			}

			if (setDate)
			{
				rtc.halt(false);
				rtc.writeProtect(false);
				rtc.setDate(newTime.date, newTime.mon, newTime.year);
				rtc.writeProtect(true);
				rtc.halt(true);

				DelArrows();
				lcd.setCursor(0, 1);
				lcd.print("  ");
				lcd.setCursor(14, 1);
				lcd.print("  ");
			}

			if (setAlarm)
			{
				if (!TimeEqual(alarmTime, newTime))
				{
					alarmTime.hour = newTime.hour;
					alarmTime.min = newTime.min;
					alarmTime.sec = newTime.sec;

					lcd.setCursor(0, 0);
					lcd.print("AC");

					alarmOn = true;
				}
				else
				{
					lcd.setCursor(0, 0);
					lcd.print("  ");

					alarmOn = false;
				}

				DelArrows();
				LcdClearLine(1);
			}

			setTime = false;
			setDate = false;
			setAlarm = false;
		}
	}

	CurrentMode->function();
}

//----------------

//----------------------------------
//-------Init cycle list mode------
//----------------------------------
void InitModeList()
{
	ModeList* first = (ModeList*)malloc(sizeof(ModeList));
	CurrentMode = first;

	CurrentMode->function = &DefaultMode;
	CurrentMode->next = (ModeList*)malloc(sizeof(ModeList));
	CurrentMode = CurrentMode->next;

	CurrentMode->function = &SetTimeMode;
	CurrentMode->next = (ModeList*)malloc(sizeof(ModeList));
	CurrentMode = CurrentMode->next;

	CurrentMode->function = &SetDateMode;
	CurrentMode->next = (ModeList*)malloc(sizeof(ModeList));
	CurrentMode = CurrentMode->next;

	CurrentMode->function = &SetAlarmMode;
	CurrentMode->next = first;
	CurrentMode = first;
}

//----------------

//----------------------------------
//--------Default clock mode--------
//----------------------------------
void DefaultMode()
{
	static unsigned long int curInt = 0;
	static unsigned long int lastInt = millis();
	static bool flag = true;

	if (beepOn)
	{
		if (CheckButton(BTN_SELECT, 2000))
		{
			digitalWrite(BEEP_PIN, HIGH);
			lcd.setCursor(0, 0);
			lcd.print("  ");
			beepOn = false;
		}
	}

	curInt = millis();
	if (max(curInt, lastInt) - min(curInt, lastInt) > 500)
	{
		lastInt = curInt;

		Time curTime = rtc.getTime();

		if (alarmOn)
		{
			if (TimeEqual(alarmTime, curTime))
			{
				alarmOn = false;
				beepOn = true;
			}
		}

		if (flag)
		{
			lcd.setCursor(4, 0);
			LcdPrintNum((int)curTime.hour);
			lcd.print(':');
			LcdPrintNum((int)curTime.min);
			lcd.print(':');
			LcdPrintNum((int)curTime.sec);

			lcd.setCursor(3, 1);
			LcdPrintNum((int)curTime.date);
			lcd.print('.');
			LcdPrintNum((int)curTime.mon);
			lcd.print('.');
			lcd.print(curTime.year);
		}

		if (beepOn)
		{
			digitalWrite(BEEP_PIN, flag);
		}

		flag = !flag;
	}
}

//----------------

//----------------------------------
//--------Set time clock mode-------
//----------------------------------
void SetTimeMode()
{
	if (!setTime)
	{
		newTime = rtc.getTime();
		oldTime = rtc.getTime();

		DelArrows();
		selectMode = SET_HOUR;
		PrintArrows();

		lcd.setCursor(0, 0);
		lcd.print("->");
		lcd.setCursor(14, 0);
		lcd.print("<-");

		setTime = true;
	}

	if (CheckButton(BTN_LEFT, 100) && selectMode != SET_HOUR)
	{
		DelArrows();
		selectMode--;
		PrintArrows();
	}

	if (CheckButton(BTN_RIGHT, 100) && selectMode != SET_SEC)
	{
		DelArrows();
		selectMode++;
		PrintArrows();
	}

	if (CheckButton(BTN_UP, 100))
	{
		switch (selectMode)
		{
		case SET_HOUR:
			if (newTime.hour >= 0 && newTime.hour < 23)
			{
				SetCursorCrd();
				newTime.hour += 1;
				LcdPrintNum((int)newTime.hour);
			}
			return;

		case SET_MIN:
			if (newTime.min >= 0 && newTime.min < 59)
			{
				SetCursorCrd();
				newTime.min += 1;
				LcdPrintNum((int)newTime.min);
			}
			return;

		case SET_SEC:
			if (newTime.sec >= 0 && newTime.sec < 59)
			{
				SetCursorCrd();
				newTime.sec += 1;
				LcdPrintNum((int)newTime.sec);
			}
			return;
		}
	}

	if (CheckButton(BTN_DOWN, 100))
	{
		switch (selectMode)
		{
		case SET_HOUR:
			if (newTime.hour > 0 && newTime.hour <= 23)
			{
				SetCursorCrd();
				newTime.hour -= 1;
				LcdPrintNum((int)newTime.hour);
			}
			return;

		case SET_MIN:
			if (newTime.min > 0 && newTime.min <= 59)
			{
				SetCursorCrd();
				newTime.min -= 1;
				LcdPrintNum((int)newTime.min);
			}
			return;

		case SET_SEC:
			if (newTime.sec > 0 && newTime.sec <= 59)
			{
				SetCursorCrd();
				newTime.sec -= 1;
				LcdPrintNum((int)newTime.sec);
			}
			return;
		}
	}
}

//----------------

//----------------------------------
//-----------Set date mode----------
//----------------------------------
void SetDateMode()
{
	if (!setDate)
	{
		newTime = rtc.getTime();

		DelArrows();
		selectMode = SET_DATE;
		PrintArrows();

		lcd.setCursor(0, 1);
		lcd.print("->");
		lcd.setCursor(14, 1);
		lcd.print("<-");

		setDate = true;
	}

	if (CheckButton(BTN_LEFT, 100) && selectMode != SET_DATE)
	{
		DelArrows();
		selectMode--;
		PrintArrows();
	}

	if (CheckButton(BTN_RIGHT, 100) && selectMode != SET_YEAR)
	{
		DelArrows();
		selectMode++;
		PrintArrows();
	}

	if (CheckButton(BTN_UP, 100))
	{
		switch (selectMode)
		{
		case SET_DATE:
			if (newTime.date >= 1 && newTime.date < 31)
			{
				SetCursorCrd();
				newTime.date += 1;
				LcdPrintNum((int)newTime.date);
			}
			return;

		case SET_MON:
			if (newTime.mon >= 1 && newTime.mon < 12)
			{
				SetCursorCrd();
				newTime.mon += 1;
				LcdPrintNum((int)newTime.mon);
			}
			return;

		case SET_YEAR:
			if (newTime.year >= 1000 && newTime.year < 9999)
			{
				SetCursorCrd();
				newTime.year += 1;
				LcdPrintNum((int)newTime.year);
			}
			return;
		}
	}

	if (CheckButton(BTN_DOWN, 100))
	{
		switch (selectMode)
		{
		case SET_DATE:
			if (newTime.date > 1 && newTime.date <= 31)
			{
				SetCursorCrd();
				newTime.date -= 1;
				LcdPrintNum((int)newTime.date);
			}
			return;

		case SET_MON:
			if (newTime.mon > 1 && newTime.mon <= 12)
			{
				SetCursorCrd();
				newTime.mon -= 1;
				LcdPrintNum((int)newTime.mon);
			}
			return;

		case SET_YEAR:
			if (newTime.year > 1000 && newTime.year <= 9999)
			{
				SetCursorCrd();
				newTime.year -= 1;
				LcdPrintNum((int)newTime.year);
			}
			return;
		}
	}
}

//----------------

//----------------------------------
//-------Set alarm clock mode-------
//----------------------------------
void SetAlarmMode()
{
	if (!setAlarm)
	{
		newTime = rtc.getTime();
		alarmTime = rtc.getTime();

		DelArrows();
		selectMode = SET_HOUR;

		lcd.setCursor(3, 0);
		lcd.write(byte(1));
		LcdPrintNum((int)newTime.hour);
		lcd.write(byte(0));
		LcdPrintNum((int)newTime.min);
		lcd.print(':');
		LcdPrintNum((int)newTime.sec);


		LcdClearLine(1);
		lcd.setCursor(2, 1);
		lcd.print("ALARM CLOCK");

		setAlarm = true;
		setTime = true;
	}

	SetTimeMode();
}

//----------------

//----------------------------------
//--------Check button pushed-------
//----------------------------------
bool CheckButton(ButtonID id, unsigned int latency)
{
	int key = analogRead(A0);

	curIntBtn[id] = millis();
	if (key > analogRange[id] && key < analogRange[id + 1])
	{
		if (max(curIntBtn[id], lastIntBtn[id]) - min(curIntBtn[id], lastIntBtn[id]) > latency)
		{
			lastIntBtn[id] = curIntBtn[id];
			return (true);
		}
	}
	else
	{
		lastIntBtn[id] = curIntBtn[id];
	}

	return (false);
}

//----------------

//----------------------------------
//-------Time equal function--------
//----------------------------------
bool TimeEqual(Time t1, Time t2)
{
	if (t1.hour == t2.hour)
	{
		if (t1.min == t2.min)
		{
			if (t1.sec == t2.sec)
			{
				return (true);
			}
		}
	}

	return (false);
}

//----------------

//----------------------------------
//-------Clear line on display------
//----------------------------------
void LcdClearLine(uint8_t line)
{
	lcd.setCursor(0, line);
	lcd.print("                ");
}

//----------------

//----------------------------------
//-------Print num on display-------
//----------------------------------
void LcdPrintNum(int num)
{
	if (num / 10 == 0)
		lcd.print('0');
	lcd.print(num);
}

//----------------

//----------------------------------
//------Print arrows on display-----
//----------------------------------
void PrintArrows()
{
	switch (selectMode)
	{
	case SET_HOUR:
		lcd.setCursor(CRD_HOUR - 1, 0);
		lcd.write(byte(1));
		lcd.setCursor(CRD_HOUR + 2, 0);
		lcd.write(byte(0));
		return;

	case SET_MIN:
		lcd.setCursor(CRD_MIN - 1, 0);
		lcd.write(byte(1));
		lcd.setCursor(CRD_MIN + 2, 0);
		lcd.write(byte(0));
		return;

	case SET_SEC:
		lcd.setCursor(CRD_SEC - 1, 0);
		lcd.write(byte(1));
		lcd.setCursor(CRD_SEC + 2, 0);
		lcd.write(byte(0));
		return;

	case SET_DATE:
		lcd.setCursor(CRD_DATE - 1, 1);
		lcd.write(byte(1));
		lcd.setCursor(CRD_DATE + 2, 1);
		lcd.write(byte(0));
		return;

	case SET_MON:
		lcd.setCursor(CRD_MON - 1, 1);
		lcd.write(byte(1));
		lcd.setCursor(CRD_MON + 2, 1);
		lcd.write(byte(0));
		return;

	case SET_YEAR:
		lcd.setCursor(CRD_YEAR - 1, 1);
		lcd.write(byte(1));
		lcd.setCursor(CRD_YEAR + 4, 1);
		lcd.write(byte(0));
		return;
	}
}

//----------------

//----------------------------------
//----Delete arrows from display----
//----------------------------------
void DelArrows()
{
	switch (selectMode)
	{
	case SET_HOUR:
		lcd.setCursor(CRD_HOUR - 1, 0);
		lcd.print(' ');
		lcd.setCursor(CRD_HOUR + 2, 0);
		lcd.print(':');
		return;

	case SET_MIN:
		lcd.setCursor(CRD_MIN - 1, 0);
		lcd.print(':');
		lcd.setCursor(CRD_MIN + 2, 0);
		lcd.print(':');
		return;

	case SET_SEC:
		lcd.setCursor(CRD_SEC - 1, 0);
		lcd.print(':');
		lcd.setCursor(CRD_SEC + 2, 0);
		lcd.print(' ');
		return;

	case SET_DATE:
		lcd.setCursor(CRD_DATE - 1, 1);
		lcd.print(' ');
		lcd.setCursor(CRD_DATE + 2, 1);
		lcd.print('.');
		return;

	case SET_MON:
		lcd.setCursor(CRD_MON - 1, 1);
		lcd.print('.');
		lcd.setCursor(CRD_MON + 2, 1);
		lcd.print('.');
		return;

	case SET_YEAR:
		lcd.setCursor(CRD_YEAR - 1, 1);
		lcd.print('.');
		lcd.setCursor(CRD_YEAR + 4, 1);
		lcd.print(' ');
		return;
	}
}

//----------------

//----------------------------------
//-------Set cursor on display------
//----------------------------------
void SetCursorCrd()
{
	switch (selectMode)
	{
	case SET_HOUR:
		lcd.setCursor(CRD_HOUR, 0);
		return;

	case SET_MIN:
		lcd.setCursor(CRD_MIN, 0);
		return;

	case SET_SEC:
		lcd.setCursor(CRD_SEC, 0);
		return;

	case SET_DATE:
		lcd.setCursor(CRD_DATE, 1);
		return;

	case SET_MON:
		lcd.setCursor(CRD_MON, 1);
		return;

	case SET_YEAR:
		lcd.setCursor(CRD_YEAR, 1);
		return;
	}
}

//----------------


//-----------------------------
