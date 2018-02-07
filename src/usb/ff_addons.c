
#include "diskio.h"
#include "integer.h"

union
{
	fat_time_t fat_time;
	DWORD dword;
}ftime;

// вернуть системное время
DWORD get_fattime(void);


//*-----------------------------------------------------------------------------------------------
/**			Возврат текущего системного времени
 * @return - время, 32-битное слово в формате времени FAT (см. тип fat_time_t)					*/
//*-----------------------------------------------------------------------------------------------
DWORD get_fattime(void)
{
	// возвращаем константное время
	ftime.fat_time.year = 2013-1980;	// начало отсчёта 1980 год
	ftime.fat_time.month = 9;
	ftime.fat_time.day = 4;
	ftime.fat_time.hour = 12;
	ftime.fat_time.min = 0;
	ftime.fat_time.sec = 0;
	return ftime.dword;
}
