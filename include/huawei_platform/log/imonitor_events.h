/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
WARNING: This header file is auto-generated, DO NOT modify it manually
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

#ifndef _IMONITOR_EVENTS_H_
#define _IMONITOR_EVENTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* event index struct */
struct imonitor_event_index {
	unsigned int eventid;
	const unsigned short *desc;
	unsigned  int params_count;
};

/* global index struct */
struct imonitor_module_index {
	const struct imonitor_event_index *events;
	unsigned  int min_eventid;
	unsigned  int max_eventid;
	unsigned  int events_count;
};

/*the event array describes the params' type table of each event
  Key 's type define below:
  bit:1
  tinyint:2
  smallint:3
  int:4
  float:5
  datetime: 6
  varchar: larger than 100
  for varchar type is special, any integer larger than 100
  means this key type is varchar and the max length is (type-100)
  for example:
  int E100000000 = {4,164}}
  that means the event 100000000's 1st parmameter type is int
  the 2nd parameter type is varchar, max length is 64 (164-100)
  type in (6,100] is invalid
*/

/*** Reliability events ***/
const unsigned short E901000000[3]
	 = {164, 228, 228};
const unsigned short E901000001[2]
	 = {164, 164};
const unsigned short E901000002[5]
	 = {164, 164, 228, 228, 356};
const unsigned short E901000003[3]
	 = {164, 228, 356};
const unsigned short E901000004[3]
	 = {164, 228, 356};
const unsigned short E901000005[3]
	 = {164, 228, 356};
const unsigned short E901001000[7]
	 = {164, 228, 164, 4, 4, 164, 164};
const unsigned short E901001001[5]
	 = {164, 228, 228, 164, 164};
const unsigned short E901001002[5]
	 = {164, 228, 4, 164, 164};
const unsigned short E901002000[2]
	 = {164, 228};
const unsigned short E901002001[2]
	 = {164, 228};
const unsigned short E901002002[2]
	 = {164, 228};
const unsigned short E901002003[2]
	 = {164, 228};
const unsigned short E901002004[2]
	 = {164, 228};
const unsigned short E901003000[5]
	 = {164, 164, 4, 164, 164};
const unsigned short E901003001[5]
	 = {164, 164, 4, 164, 164};
const unsigned short E901003002[3]
	 = {164, 164, 164};
const unsigned short E901004000[6]
	 = {164, 164, 4, 4, 228, 356};
const unsigned short E901004001[6]
	 = {164, 164, 4, 4, 228, 356};
const unsigned short E901005000[5]
	 = {4, 132, 132, 132, 132};
const struct imonitor_event_index Reliability_events[20]
	= { {901000000, E901000000, 3}, {901000001, E901000001, 2},
		{901000002, E901000002, 5}, {901000003, E901000003, 3},
		{901000004, E901000004, 3}, {901000005, E901000005, 3},
		{901001000, E901001000, 7}, {901001001, E901001001, 5},
		{901001002, E901001002, 5}, {901002000, E901002000, 2},
		{901002001, E901002001, 2}, {901002002, E901002002, 2},
		{901002003, E901002003, 2}, {901002004, E901002004, 2},
		{901003000, E901003000, 5}, {901003001, E901003001, 5},
		{901003002, E901003002, 3}, {901004000, E901004000, 6},
		{901004001, E901004001, 6}, {901005000, E901005000, 5} };

/*** Performance events ***/
const unsigned short E902000000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902000001[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902000002[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902000003[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902010000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902010001[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902010002[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902010003[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902040000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902040100[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050001[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050002[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050003[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050004[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050005[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050006[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050007[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050008[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050009[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050010[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050011[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050012[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902050100[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902060000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902060001[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902060002[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902060003[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902060100[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902070000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902070001[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902070002[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902070003[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902070100[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902080000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902090000[23]
	 = {164, 4, 4, 228, 4, 164, 4, 164, 4, 164,
		4, 164, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 132};
const unsigned short E902500000[24]
	 = {164, 4, 228, 132, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4};
const unsigned short E902500001[24]
	 = {164, 4, 228, 132, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4};
const unsigned short E902500002[24]
	 = {164, 4, 228, 132, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4};
const unsigned short E902500003[24]
	 = {164, 4, 228, 132, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4};
const unsigned short E902500004[24]
	 = {164, 4, 228, 132, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4};
const struct imonitor_event_index Performance_events[41]
	= { {902000000, E902000000, 23}, {902000001, E902000001, 23},
		{902000002, E902000002, 23}, {902000003, E902000003, 23},
		{902010000, E902010000, 23}, {902010001, E902010001, 23},
		{902010002, E902010002, 23}, {902010003, E902010003, 23},
		{902040000, E902040000, 23}, {902040100, E902040100, 23},
		{902050000, E902050000, 23}, {902050001, E902050001, 23},
		{902050002, E902050002, 23}, {902050003, E902050003, 23},
		{902050004, E902050004, 23}, {902050005, E902050005, 23},
		{902050006, E902050006, 23}, {902050007, E902050007, 23},
		{902050008, E902050008, 23}, {902050009, E902050009, 23},
		{902050010, E902050010, 23}, {902050011, E902050011, 23},
		{902050012, E902050012, 23}, {902050100, E902050100, 23},
		{902060000, E902060000, 23}, {902060001, E902060001, 23},
		{902060002, E902060002, 23}, {902060003, E902060003, 23},
		{902060100, E902060100, 23}, {902070000, E902070000, 23},
		{902070001, E902070001, 23}, {902070002, E902070002, 23},
		{902070003, E902070003, 23}, {902070100, E902070100, 23},
		{902080000, E902080000, 23}, {902090000, E902090000, 23},
		{902500000, E902500000, 24}, {902500001, E902500001, 24},
		{902500002, E902500002, 24}, {902500003, E902500003, 24},
		{902500004, E902500004, 24} };

/*** PowerThermal events ***/
const unsigned short E903001000[19]
	 = {6, 6, 164, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 164};
const unsigned short E903001001[7]
	 = {6, 6, 4, 4, 4, 4, 4};
const unsigned short E903001002[7]
	 = {6, 6, 4, 4, 4, 4, 4};
const unsigned short E903001003[8]
	 = {6, 6, 132, 132, 356, 356, 4, 4};
const unsigned short E903001004[6]
	 = {6, 6, 132, 356, 4, 4};
const unsigned short E903001005[4]
	 = {6, 6, 356, 4};
const unsigned short E903001006[6]
	 = {6, 6, 5, 5, 5, 5};
const unsigned short E903001007[4]
	 = {6, 6, 356, 4};
const unsigned short E903001008[5]
	 = {6, 6, 356, 4, 4};
const unsigned short E903001009[14]
	 = {6, 6, 164, 356, 5, 4, 4, 4, 4, 4,
		4, 5, 5, 5};
const unsigned short E903001010[3]
	 = {6, 132, 132};
const unsigned short E903001011[12]
	 = {6, 2, 4, 4, 2, 2, 2, 2, 4, 4,
		4, 4};
const unsigned short E903001012[10]
	 = {6, 120, 2, 2, 2, 2, 2, 2, 2, 2,
		};
const unsigned short E903001013[3]
	 = {6, 356, 4};
const unsigned short E903001014[6]
	 = {6, 612, 356, 4, 4, 228};
const unsigned short E903001015[14]
	 = {6, 6, 8100, 8100, 8100, 8100, 8100, 8100, 8100, 8100,
		8100, 8100, 8100, 8100};
const unsigned short E903001016[6]
	 = {6, 6, 164, 4, 4, 5};
const unsigned short E903001017[26]
	 = {6, 6, 3, 3, 4, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4};
const unsigned short E903001018[25]
	 = {6, 6, 3, 3, 4, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4};
const unsigned short E903001019[26]
	 = {6, 6, 3, 3, 4, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4};
const unsigned short E903001020[20]
	 = {6, 6, 3, 3, 4, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		};
const unsigned short E903001021[27]
	 = {6, 6, 3, 3, 4, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4};
const unsigned short E903001022[24]
	 = {6, 6, 3, 3, 4, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4};
const unsigned short E903001023[15]
	 = {6, 6, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 300, 200, 228};
const unsigned short E903002000[4]
	 = {164, 4, 4, 6};
const unsigned short E903002001[12]
	 = {164, 4, 4, 4, 4, 4, 4, 4, 4, 164,
		4, 6};
const unsigned short E903002002[13]
	 = {4, 4, 4, 4, 4, 4, 164, 164, 4, 164,
		4, 4, 6};
const unsigned short E903002003[3]
	 = {164, 164, 6};
const unsigned short E903002004[4]
	 = {4, 4, 164, 6};
const unsigned short E903002005[6]
	 = {164, 4, 4, 164, 228, 6};
const unsigned short E903002006[12]
	 = {228, 4, 228, 4, 228, 4, 164, 4, 164, 164,
		4, 6};
const unsigned short E903002007[6]
	 = {4, 228, 4, 228, 4, 6};
const unsigned short E903002008[13]
	 = {4, 4, 228, 4, 4, 2, 2, 2, 2, 2,
		2, 2, 6};
const unsigned short E903002009[6]
	 = {4, 4, 2, 132, 132, 6};
const unsigned short E903002010[27]
	 = {125, 120, 2, 2, 2, 2, 2, 2, 2, 2,
		4, 4, 4, 4, 4, 2, 5, 4, 4, 4,
		2, 130, 3, 2, 2, 2, 600};
const unsigned short E903003000[8]
	 = {6, 6, 4, 4, 6, 6, 164, 4};
const struct imonitor_event_index PowerThermal_events[36]
	= { {903001000, E903001000, 19}, {903001001, E903001001, 7},
		{903001002, E903001002, 7}, {903001003, E903001003, 8},
		{903001004, E903001004, 6}, {903001005, E903001005, 4},
		{903001006, E903001006, 6}, {903001007, E903001007, 4},
		{903001008, E903001008, 5}, {903001009, E903001009, 14},
		{903001010, E903001010, 3}, {903001011, E903001011, 12},
		{903001012, E903001012, 10}, {903001013, E903001013, 3},
		{903001014, E903001014, 6}, {903001015, E903001015, 14},
		{903001016, E903001016, 6}, {903001017, E903001017, 26},
		{903001018, E903001018, 25}, {903001019, E903001019, 26},
		{903001020, E903001020, 20}, {903001021, E903001021, 27},
		{903001022, E903001022, 24}, {903001023, E903001023, 15},
		{903002000, E903002000, 4}, {903002001, E903002001, 12},
		{903002002, E903002002, 13}, {903002003, E903002003, 3},
		{903002004, E903002004, 4}, {903002005, E903002005, 6},
		{903002006, E903002006, 12}, {903002007, E903002007, 6},
		{903002008, E903002008, 13}, {903002009, E903002009, 6},
		{903002010, E903002010, 27}, {903003000, E903003000, 8} };

/*** CHR events ***/
const unsigned short E904000000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904001000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904002000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904003000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904004000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904004001[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904005000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904006000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904007000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904008000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904009000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904010000[5]
	 = {4, 4, 4, 4, 4};
const unsigned short E904100000[1]
	 = {0};
const struct imonitor_event_index CHR_events[13]
	= { {904000000, E904000000, 5}, {904001000, E904001000, 5},
		{904002000, E904002000, 5}, {904003000, E904003000, 5},
		{904004000, E904004000, 5}, {904004001, E904004001, 5},
		{904005000, E904005000, 5}, {904006000, E904006000, 5},
		{904007000, E904007000, 5}, {904008000, E904008000, 5},
		{904009000, E904009000, 5}, {904010000, E904010000, 5},
		{904100000, E904100000, 0} };

/*** DMD events ***/
const unsigned short E920001003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001009[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001011[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001012[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001013[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001014[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001015[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001016[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001017[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001018[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001019[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001021[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001022[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001023[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001024[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001025[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001026[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001027[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001030[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001060[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001061[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001062[4]
	 = {4, 164, 164, 1124};
const unsigned short E920001063[4]
	 = {4, 164, 164, 1124};
const unsigned short E920002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920003000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920004000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920004001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005009[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005011[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005012[4]
	 = {4, 164, 164, 1124};
const unsigned short E920005013[4]
	 = {4, 164, 164, 1124};
const unsigned short E920006000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920006001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920006002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007009[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007011[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007012[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007013[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007014[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007015[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007016[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007017[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007018[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007019[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007020[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007021[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007022[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007023[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007024[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007025[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007026[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007027[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007028[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007029[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007030[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007031[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007032[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007033[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007034[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007035[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007036[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007037[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007038[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007039[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007040[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007041[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007042[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007043[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007044[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007045[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007046[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007047[4]
	 = {4, 164, 164, 1124};
const unsigned short E920007048[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008011[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008012[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008013[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008014[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008015[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008016[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008017[4]
	 = {4, 164, 164, 1124};
const unsigned short E920008030[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009009[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009011[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009012[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009013[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009014[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009015[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009016[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009017[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009018[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009019[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009020[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009021[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009022[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009023[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009024[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009025[4]
	 = {4, 164, 164, 1124};
const unsigned short E920009026[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010009[4]
	 = {4, 164, 164, 1124};
const unsigned short E920010010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011000[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011001[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011002[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011003[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011004[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011005[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011006[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011007[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011008[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011009[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011010[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011011[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011012[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011013[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011014[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011015[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011016[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011017[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011018[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011019[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011020[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011021[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011022[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011023[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011024[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011025[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011026[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011027[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011028[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011029[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011030[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011031[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011032[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011033[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011034[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011035[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011036[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011037[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011038[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011039[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011040[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011041[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011042[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011043[4]
	 = {4, 164, 164, 1124};
const unsigned short E920011044[4]
	 = {4, 164, 164, 1124};
const unsigned short E920012000[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001002[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001003[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001004[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001005[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001006[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001007[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001008[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001009[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001010[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001011[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001012[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001013[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001014[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001015[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001016[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001017[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001018[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001019[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001020[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001021[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001022[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001023[4]
	 = {4, 164, 164, 1124};
const unsigned short E921001024[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002003[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002004[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002005[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002006[4]
	 = {4, 164, 164, 1124};
const unsigned short E921002007[4]
	 = {4, 164, 164, 1124};
const unsigned short E921003000[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001002[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001003[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001004[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001005[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001006[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001007[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001008[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001009[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001010[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001011[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001012[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001013[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001014[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001015[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001016[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001017[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001018[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001019[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001020[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001021[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001022[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001023[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001024[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001025[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001026[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001027[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001028[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001029[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001030[4]
	 = {4, 164, 164, 1124};
const unsigned short E922001031[4]
	 = {4, 164, 164, 1124};
const unsigned short E922002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E922002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E922002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E923001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002003[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002004[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002005[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002006[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002007[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002008[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002009[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002010[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002011[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002012[4]
	 = {4, 164, 164, 1124};
const unsigned short E923002013[4]
	 = {4, 164, 164, 1124};
const unsigned short E924001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E924002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E924003000[4]
	 = {4, 164, 164, 1124};
const unsigned short E924003001[4]
	 = {4, 164, 164, 1124};
const unsigned short E925001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E925001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E925001002[4]
	 = {4, 164, 164, 1124};
const unsigned short E925001003[4]
	 = {4, 164, 164, 1124};
const unsigned short E925001004[4]
	 = {4, 164, 164, 1124};
const unsigned short E925001005[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002003[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002004[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002005[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002006[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002007[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002008[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002009[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002010[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002011[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002012[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002013[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002014[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002015[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002016[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002017[4]
	 = {4, 164, 164, 1124};
const unsigned short E925002018[4]
	 = {4, 164, 164, 1124};
const unsigned short E926001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926001002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926001003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926001004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926001005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926003006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004007[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004008[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004009[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004010[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004011[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004012[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004013[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004014[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004015[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004016[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004017[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004018[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004019[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004020[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004021[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004022[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004023[4]
	 = {4, 164, 164, 1124};
const unsigned short E926004024[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005007[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005008[4]
	 = {4, 164, 164, 1124};
const unsigned short E926005009[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006007[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006008[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006009[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006010[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006011[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006012[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006013[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006100[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006101[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006102[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006103[4]
	 = {4, 164, 164, 1124};
const unsigned short E926006104[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007007[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007008[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007009[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007010[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007011[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007012[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007013[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007014[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007015[4]
	 = {4, 164, 164, 1124};
const unsigned short E926007016[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008007[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008008[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008009[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008010[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008011[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008012[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008013[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008014[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008015[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008016[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008017[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008018[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008019[4]
	 = {4, 164, 164, 1124};
const unsigned short E926008020[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009000[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009001[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009002[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009003[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009004[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009005[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009006[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009007[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009008[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009009[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009010[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009011[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009012[4]
	 = {4, 164, 164, 1124};
const unsigned short E926009013[4]
	 = {4, 164, 164, 1124};
const unsigned short E927001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927002000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003003[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003004[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003005[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003006[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003007[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003008[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003009[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003010[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003011[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003012[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003013[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003014[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003015[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003016[4]
	 = {4, 164, 164, 1124};
const unsigned short E927003017[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004003[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004004[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004005[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004006[4]
	 = {4, 164, 164, 1124};
const unsigned short E927004007[4]
	 = {4, 164, 164, 1124};
const unsigned short E927005000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927005001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927005002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927005003[4]
	 = {4, 164, 164, 1124};
const unsigned short E927005004[4]
	 = {4, 164, 164, 1124};
const unsigned short E927005005[4]
	 = {4, 164, 164, 1124};
const unsigned short E927006000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927006001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927006002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927006003[4]
	 = {4, 164, 164, 1124};
const unsigned short E927006004[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007003[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007004[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007005[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007006[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007007[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007008[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007009[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007010[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007011[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007012[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007013[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007014[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007015[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007016[4]
	 = {4, 164, 164, 1124};
const unsigned short E927007017[4]
	 = {4, 164, 164, 1124};
const unsigned short E927008000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009000[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009001[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009002[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009003[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009004[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009005[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009006[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009007[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009008[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009009[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009010[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009011[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009012[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009013[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009014[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009015[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009016[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009017[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009018[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009019[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009020[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009021[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009022[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009023[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009024[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009025[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009026[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009027[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009028[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009029[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009030[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009031[4]
	 = {4, 164, 164, 1124};
const unsigned short E927009032[4]
	 = {4, 164, 164, 1124};
const unsigned short E928001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002003[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002004[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002005[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002006[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002007[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002008[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002009[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002010[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002011[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002012[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002013[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002014[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002015[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002016[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002017[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002018[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002019[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002020[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002021[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002022[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002023[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002024[4]
	 = {4, 164, 164, 1124};
const unsigned short E928002025[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003002[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003003[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003004[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003005[4]
	 = {4, 164, 164, 1124};
const unsigned short E928003006[4]
	 = {4, 164, 164, 1124};
const unsigned short E928004000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928005000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928005001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928005002[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006002[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006003[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006004[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006005[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006006[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006007[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006008[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006009[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006010[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006011[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006012[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006013[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006014[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006015[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006016[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006017[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006018[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006019[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006020[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006021[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006022[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006023[4]
	 = {4, 164, 164, 1124};
const unsigned short E928006024[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007002[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007003[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007004[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007005[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007006[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007007[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007008[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007009[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007010[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007011[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007012[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007013[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007014[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007015[4]
	 = {4, 164, 164, 1124};
const unsigned short E928007016[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008000[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008001[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008002[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008003[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008004[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008005[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008006[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008007[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008008[4]
	 = {4, 164, 164, 1124};
const unsigned short E928008009[4]
	 = {4, 164, 164, 1124};
const struct imonitor_event_index DMD_events[620]
	= { {920001003, E920001003, 4}, {920001004, E920001004, 4},
		{920001005, E920001005, 4}, {920001006, E920001006, 4},
		{920001007, E920001007, 4}, {920001008, E920001008, 4},
		{920001009, E920001009, 4}, {920001010, E920001010, 4},
		{920001011, E920001011, 4}, {920001012, E920001012, 4},
		{920001013, E920001013, 4}, {920001014, E920001014, 4},
		{920001015, E920001015, 4}, {920001016, E920001016, 4},
		{920001017, E920001017, 4}, {920001018, E920001018, 4},
		{920001019, E920001019, 4}, {920001021, E920001021, 4},
		{920001022, E920001022, 4}, {920001023, E920001023, 4},
		{920001024, E920001024, 4}, {920001025, E920001025, 4},
		{920001026, E920001026, 4}, {920001027, E920001027, 4},
		{920001030, E920001030, 4}, {920001060, E920001060, 4},
		{920001061, E920001061, 4}, {920001062, E920001062, 4},
		{920001063, E920001063, 4}, {920002001, E920002001, 4},
		{920003000, E920003000, 4}, {920004000, E920004000, 4},
		{920004001, E920004001, 4}, {920005000, E920005000, 4},
		{920005001, E920005001, 4}, {920005002, E920005002, 4},
		{920005003, E920005003, 4}, {920005004, E920005004, 4},
		{920005005, E920005005, 4}, {920005006, E920005006, 4},
		{920005007, E920005007, 4}, {920005008, E920005008, 4},
		{920005009, E920005009, 4}, {920005010, E920005010, 4},
		{920005011, E920005011, 4}, {920005012, E920005012, 4},
		{920005013, E920005013, 4}, {920006000, E920006000, 4},
		{920006001, E920006001, 4}, {920006002, E920006002, 4},
		{920007000, E920007000, 4}, {920007001, E920007001, 4},
		{920007002, E920007002, 4}, {920007003, E920007003, 4},
		{920007004, E920007004, 4}, {920007005, E920007005, 4},
		{920007006, E920007006, 4}, {920007007, E920007007, 4},
		{920007008, E920007008, 4}, {920007009, E920007009, 4},
		{920007010, E920007010, 4}, {920007011, E920007011, 4},
		{920007012, E920007012, 4}, {920007013, E920007013, 4},
		{920007014, E920007014, 4}, {920007015, E920007015, 4},
		{920007016, E920007016, 4}, {920007017, E920007017, 4},
		{920007018, E920007018, 4}, {920007019, E920007019, 4},
		{920007020, E920007020, 4}, {920007021, E920007021, 4},
		{920007022, E920007022, 4}, {920007023, E920007023, 4},
		{920007024, E920007024, 4}, {920007025, E920007025, 4},
		{920007026, E920007026, 4}, {920007027, E920007027, 4},
		{920007028, E920007028, 4}, {920007029, E920007029, 4},
		{920007030, E920007030, 4}, {920007031, E920007031, 4},
		{920007032, E920007032, 4}, {920007033, E920007033, 4},
		{920007034, E920007034, 4}, {920007035, E920007035, 4},
		{920007036, E920007036, 4}, {920007037, E920007037, 4},
		{920007038, E920007038, 4}, {920007039, E920007039, 4},
		{920007040, E920007040, 4}, {920007041, E920007041, 4},
		{920007042, E920007042, 4}, {920007043, E920007043, 4},
		{920007044, E920007044, 4}, {920007045, E920007045, 4},
		{920007046, E920007046, 4}, {920007047, E920007047, 4},
		{920007048, E920007048, 4}, {920008000, E920008000, 4},
		{920008001, E920008001, 4}, {920008002, E920008002, 4},
		{920008003, E920008003, 4}, {920008004, E920008004, 4},
		{920008005, E920008005, 4}, {920008006, E920008006, 4},
		{920008007, E920008007, 4}, {920008008, E920008008, 4},
		{920008010, E920008010, 4}, {920008011, E920008011, 4},
		{920008012, E920008012, 4}, {920008013, E920008013, 4},
		{920008014, E920008014, 4}, {920008015, E920008015, 4},
		{920008016, E920008016, 4}, {920008017, E920008017, 4},
		{920008030, E920008030, 4}, {920009000, E920009000, 4},
		{920009001, E920009001, 4}, {920009002, E920009002, 4},
		{920009003, E920009003, 4}, {920009004, E920009004, 4},
		{920009005, E920009005, 4}, {920009006, E920009006, 4},
		{920009007, E920009007, 4}, {920009008, E920009008, 4},
		{920009009, E920009009, 4}, {920009010, E920009010, 4},
		{920009011, E920009011, 4}, {920009012, E920009012, 4},
		{920009013, E920009013, 4}, {920009014, E920009014, 4},
		{920009015, E920009015, 4}, {920009016, E920009016, 4},
		{920009017, E920009017, 4}, {920009018, E920009018, 4},
		{920009019, E920009019, 4}, {920009020, E920009020, 4},
		{920009021, E920009021, 4}, {920009022, E920009022, 4},
		{920009023, E920009023, 4}, {920009024, E920009024, 4},
		{920009025, E920009025, 4}, {920009026, E920009026, 4},
		{920010000, E920010000, 4}, {920010001, E920010001, 4},
		{920010002, E920010002, 4}, {920010003, E920010003, 4},
		{920010004, E920010004, 4}, {920010005, E920010005, 4},
		{920010006, E920010006, 4}, {920010007, E920010007, 4},
		{920010008, E920010008, 4}, {920010009, E920010009, 4},
		{920010010, E920010010, 4}, {920011000, E920011000, 4},
		{920011001, E920011001, 4}, {920011002, E920011002, 4},
		{920011003, E920011003, 4}, {920011004, E920011004, 4},
		{920011005, E920011005, 4}, {920011006, E920011006, 4},
		{920011007, E920011007, 4}, {920011008, E920011008, 4},
		{920011009, E920011009, 4}, {920011010, E920011010, 4},
		{920011011, E920011011, 4}, {920011012, E920011012, 4},
		{920011013, E920011013, 4}, {920011014, E920011014, 4},
		{920011015, E920011015, 4}, {920011016, E920011016, 4},
		{920011017, E920011017, 4}, {920011018, E920011018, 4},
		{920011019, E920011019, 4}, {920011020, E920011020, 4},
		{920011021, E920011021, 4}, {920011022, E920011022, 4},
		{920011023, E920011023, 4}, {920011024, E920011024, 4},
		{920011025, E920011025, 4}, {920011026, E920011026, 4},
		{920011027, E920011027, 4}, {920011028, E920011028, 4},
		{920011029, E920011029, 4}, {920011030, E920011030, 4},
		{920011031, E920011031, 4}, {920011032, E920011032, 4},
		{920011033, E920011033, 4}, {920011034, E920011034, 4},
		{920011035, E920011035, 4}, {920011036, E920011036, 4},
		{920011037, E920011037, 4}, {920011038, E920011038, 4},
		{920011039, E920011039, 4}, {920011040, E920011040, 4},
		{920011041, E920011041, 4}, {920011042, E920011042, 4},
		{920011043, E920011043, 4}, {920011044, E920011044, 4},
		{920012000, E920012000, 4}, {921001000, E921001000, 4},
		{921001001, E921001001, 4}, {921001002, E921001002, 4},
		{921001003, E921001003, 4}, {921001004, E921001004, 4},
		{921001005, E921001005, 4}, {921001006, E921001006, 4},
		{921001007, E921001007, 4}, {921001008, E921001008, 4},
		{921001009, E921001009, 4}, {921001010, E921001010, 4},
		{921001011, E921001011, 4}, {921001012, E921001012, 4},
		{921001013, E921001013, 4}, {921001014, E921001014, 4},
		{921001015, E921001015, 4}, {921001016, E921001016, 4},
		{921001017, E921001017, 4}, {921001018, E921001018, 4},
		{921001019, E921001019, 4}, {921001020, E921001020, 4},
		{921001021, E921001021, 4}, {921001022, E921001022, 4},
		{921001023, E921001023, 4}, {921001024, E921001024, 4},
		{921002000, E921002000, 4}, {921002001, E921002001, 4},
		{921002002, E921002002, 4}, {921002003, E921002003, 4},
		{921002004, E921002004, 4}, {921002005, E921002005, 4},
		{921002006, E921002006, 4}, {921002007, E921002007, 4},
		{921003000, E921003000, 4}, {922001000, E922001000, 4},
		{922001001, E922001001, 4}, {922001002, E922001002, 4},
		{922001003, E922001003, 4}, {922001004, E922001004, 4},
		{922001005, E922001005, 4}, {922001006, E922001006, 4},
		{922001007, E922001007, 4}, {922001008, E922001008, 4},
		{922001009, E922001009, 4}, {922001010, E922001010, 4},
		{922001011, E922001011, 4}, {922001012, E922001012, 4},
		{922001013, E922001013, 4}, {922001014, E922001014, 4},
		{922001015, E922001015, 4}, {922001016, E922001016, 4},
		{922001017, E922001017, 4}, {922001018, E922001018, 4},
		{922001019, E922001019, 4}, {922001020, E922001020, 4},
		{922001021, E922001021, 4}, {922001022, E922001022, 4},
		{922001023, E922001023, 4}, {922001024, E922001024, 4},
		{922001025, E922001025, 4}, {922001026, E922001026, 4},
		{922001027, E922001027, 4}, {922001028, E922001028, 4},
		{922001029, E922001029, 4}, {922001030, E922001030, 4},
		{922001031, E922001031, 4}, {922002000, E922002000, 4},
		{922002001, E922002001, 4}, {922002002, E922002002, 4},
		{923001000, E923001000, 4}, {923002000, E923002000, 4},
		{923002001, E923002001, 4}, {923002002, E923002002, 4},
		{923002003, E923002003, 4}, {923002004, E923002004, 4},
		{923002005, E923002005, 4}, {923002006, E923002006, 4},
		{923002007, E923002007, 4}, {923002008, E923002008, 4},
		{923002009, E923002009, 4}, {923002010, E923002010, 4},
		{923002011, E923002011, 4}, {923002012, E923002012, 4},
		{923002013, E923002013, 4}, {924001000, E924001000, 4},
		{924002000, E924002000, 4}, {924003000, E924003000, 4},
		{924003001, E924003001, 4}, {925001000, E925001000, 4},
		{925001001, E925001001, 4}, {925001002, E925001002, 4},
		{925001003, E925001003, 4}, {925001004, E925001004, 4},
		{925001005, E925001005, 4}, {925002000, E925002000, 4},
		{925002001, E925002001, 4}, {925002002, E925002002, 4},
		{925002003, E925002003, 4}, {925002004, E925002004, 4},
		{925002005, E925002005, 4}, {925002006, E925002006, 4},
		{925002007, E925002007, 4}, {925002008, E925002008, 4},
		{925002009, E925002009, 4}, {925002010, E925002010, 4},
		{925002011, E925002011, 4}, {925002012, E925002012, 4},
		{925002013, E925002013, 4}, {925002014, E925002014, 4},
		{925002015, E925002015, 4}, {925002016, E925002016, 4},
		{925002017, E925002017, 4}, {925002018, E925002018, 4},
		{926001000, E926001000, 4}, {926001001, E926001001, 4},
		{926001002, E926001002, 4}, {926001003, E926001003, 4},
		{926001004, E926001004, 4}, {926001005, E926001005, 4},
		{926002000, E926002000, 4}, {926002001, E926002001, 4},
		{926002002, E926002002, 4}, {926003000, E926003000, 4},
		{926003001, E926003001, 4}, {926003002, E926003002, 4},
		{926003003, E926003003, 4}, {926003004, E926003004, 4},
		{926003005, E926003005, 4}, {926003006, E926003006, 4},
		{926004000, E926004000, 4}, {926004001, E926004001, 4},
		{926004002, E926004002, 4}, {926004003, E926004003, 4},
		{926004004, E926004004, 4}, {926004005, E926004005, 4},
		{926004006, E926004006, 4}, {926004007, E926004007, 4},
		{926004008, E926004008, 4}, {926004009, E926004009, 4},
		{926004010, E926004010, 4}, {926004011, E926004011, 4},
		{926004012, E926004012, 4}, {926004013, E926004013, 4},
		{926004014, E926004014, 4}, {926004015, E926004015, 4},
		{926004016, E926004016, 4}, {926004017, E926004017, 4},
		{926004018, E926004018, 4}, {926004019, E926004019, 4},
		{926004020, E926004020, 4}, {926004021, E926004021, 4},
		{926004022, E926004022, 4}, {926004023, E926004023, 4},
		{926004024, E926004024, 4}, {926005000, E926005000, 4},
		{926005001, E926005001, 4}, {926005002, E926005002, 4},
		{926005003, E926005003, 4}, {926005004, E926005004, 4},
		{926005005, E926005005, 4}, {926005006, E926005006, 4},
		{926005007, E926005007, 4}, {926005008, E926005008, 4},
		{926005009, E926005009, 4}, {926006000, E926006000, 4},
		{926006001, E926006001, 4}, {926006002, E926006002, 4},
		{926006003, E926006003, 4}, {926006004, E926006004, 4},
		{926006005, E926006005, 4}, {926006006, E926006006, 4},
		{926006007, E926006007, 4}, {926006008, E926006008, 4},
		{926006009, E926006009, 4}, {926006010, E926006010, 4},
		{926006011, E926006011, 4}, {926006012, E926006012, 4},
		{926006013, E926006013, 4}, {926006100, E926006100, 4},
		{926006101, E926006101, 4}, {926006102, E926006102, 4},
		{926006103, E926006103, 4}, {926006104, E926006104, 4},
		{926007000, E926007000, 4}, {926007001, E926007001, 4},
		{926007002, E926007002, 4}, {926007003, E926007003, 4},
		{926007004, E926007004, 4}, {926007005, E926007005, 4},
		{926007006, E926007006, 4}, {926007007, E926007007, 4},
		{926007008, E926007008, 4}, {926007009, E926007009, 4},
		{926007010, E926007010, 4}, {926007011, E926007011, 4},
		{926007012, E926007012, 4}, {926007013, E926007013, 4},
		{926007014, E926007014, 4}, {926007015, E926007015, 4},
		{926007016, E926007016, 4}, {926008000, E926008000, 4},
		{926008001, E926008001, 4}, {926008002, E926008002, 4},
		{926008003, E926008003, 4}, {926008004, E926008004, 4},
		{926008005, E926008005, 4}, {926008006, E926008006, 4},
		{926008007, E926008007, 4}, {926008008, E926008008, 4},
		{926008009, E926008009, 4}, {926008010, E926008010, 4},
		{926008011, E926008011, 4}, {926008012, E926008012, 4},
		{926008013, E926008013, 4}, {926008014, E926008014, 4},
		{926008015, E926008015, 4}, {926008016, E926008016, 4},
		{926008017, E926008017, 4}, {926008018, E926008018, 4},
		{926008019, E926008019, 4}, {926008020, E926008020, 4},
		{926009000, E926009000, 4}, {926009001, E926009001, 4},
		{926009002, E926009002, 4}, {926009003, E926009003, 4},
		{926009004, E926009004, 4}, {926009005, E926009005, 4},
		{926009006, E926009006, 4}, {926009007, E926009007, 4},
		{926009008, E926009008, 4}, {926009009, E926009009, 4},
		{926009010, E926009010, 4}, {926009011, E926009011, 4},
		{926009012, E926009012, 4}, {926009013, E926009013, 4},
		{927001000, E927001000, 4}, {927001001, E927001001, 4},
		{927002000, E927002000, 4}, {927002001, E927002001, 4},
		{927002002, E927002002, 4}, {927003000, E927003000, 4},
		{927003001, E927003001, 4}, {927003002, E927003002, 4},
		{927003003, E927003003, 4}, {927003004, E927003004, 4},
		{927003005, E927003005, 4}, {927003006, E927003006, 4},
		{927003007, E927003007, 4}, {927003008, E927003008, 4},
		{927003009, E927003009, 4}, {927003010, E927003010, 4},
		{927003011, E927003011, 4}, {927003012, E927003012, 4},
		{927003013, E927003013, 4}, {927003014, E927003014, 4},
		{927003015, E927003015, 4}, {927003016, E927003016, 4},
		{927003017, E927003017, 4}, {927004000, E927004000, 4},
		{927004001, E927004001, 4}, {927004002, E927004002, 4},
		{927004003, E927004003, 4}, {927004004, E927004004, 4},
		{927004005, E927004005, 4}, {927004006, E927004006, 4},
		{927004007, E927004007, 4}, {927005000, E927005000, 4},
		{927005001, E927005001, 4}, {927005002, E927005002, 4},
		{927005003, E927005003, 4}, {927005004, E927005004, 4},
		{927005005, E927005005, 4}, {927006000, E927006000, 4},
		{927006001, E927006001, 4}, {927006002, E927006002, 4},
		{927006003, E927006003, 4}, {927006004, E927006004, 4},
		{927007000, E927007000, 4}, {927007001, E927007001, 4},
		{927007002, E927007002, 4}, {927007003, E927007003, 4},
		{927007004, E927007004, 4}, {927007005, E927007005, 4},
		{927007006, E927007006, 4}, {927007007, E927007007, 4},
		{927007008, E927007008, 4}, {927007009, E927007009, 4},
		{927007010, E927007010, 4}, {927007011, E927007011, 4},
		{927007012, E927007012, 4}, {927007013, E927007013, 4},
		{927007014, E927007014, 4}, {927007015, E927007015, 4},
		{927007016, E927007016, 4}, {927007017, E927007017, 4},
		{927008000, E927008000, 4}, {927009000, E927009000, 4},
		{927009001, E927009001, 4}, {927009002, E927009002, 4},
		{927009003, E927009003, 4}, {927009004, E927009004, 4},
		{927009005, E927009005, 4}, {927009006, E927009006, 4},
		{927009007, E927009007, 4}, {927009008, E927009008, 4},
		{927009009, E927009009, 4}, {927009010, E927009010, 4},
		{927009011, E927009011, 4}, {927009012, E927009012, 4},
		{927009013, E927009013, 4}, {927009014, E927009014, 4},
		{927009015, E927009015, 4}, {927009016, E927009016, 4},
		{927009017, E927009017, 4}, {927009018, E927009018, 4},
		{927009019, E927009019, 4}, {927009020, E927009020, 4},
		{927009021, E927009021, 4}, {927009022, E927009022, 4},
		{927009023, E927009023, 4}, {927009024, E927009024, 4},
		{927009025, E927009025, 4}, {927009026, E927009026, 4},
		{927009027, E927009027, 4}, {927009028, E927009028, 4},
		{927009029, E927009029, 4}, {927009030, E927009030, 4},
		{927009031, E927009031, 4}, {927009032, E927009032, 4},
		{928001000, E928001000, 4}, {928001001, E928001001, 4},
		{928002001, E928002001, 4}, {928002002, E928002002, 4},
		{928002003, E928002003, 4}, {928002004, E928002004, 4},
		{928002005, E928002005, 4}, {928002006, E928002006, 4},
		{928002007, E928002007, 4}, {928002008, E928002008, 4},
		{928002009, E928002009, 4}, {928002010, E928002010, 4},
		{928002011, E928002011, 4}, {928002012, E928002012, 4},
		{928002013, E928002013, 4}, {928002014, E928002014, 4},
		{928002015, E928002015, 4}, {928002016, E928002016, 4},
		{928002017, E928002017, 4}, {928002018, E928002018, 4},
		{928002019, E928002019, 4}, {928002020, E928002020, 4},
		{928002021, E928002021, 4}, {928002022, E928002022, 4},
		{928002023, E928002023, 4}, {928002024, E928002024, 4},
		{928002025, E928002025, 4}, {928003000, E928003000, 4},
		{928003001, E928003001, 4}, {928003002, E928003002, 4},
		{928003003, E928003003, 4}, {928003004, E928003004, 4},
		{928003005, E928003005, 4}, {928003006, E928003006, 4},
		{928004000, E928004000, 4}, {928005000, E928005000, 4},
		{928005001, E928005001, 4}, {928005002, E928005002, 4},
		{928006000, E928006000, 4}, {928006001, E928006001, 4},
		{928006002, E928006002, 4}, {928006003, E928006003, 4},
		{928006004, E928006004, 4}, {928006005, E928006005, 4},
		{928006006, E928006006, 4}, {928006007, E928006007, 4},
		{928006008, E928006008, 4}, {928006009, E928006009, 4},
		{928006010, E928006010, 4}, {928006011, E928006011, 4},
		{928006012, E928006012, 4}, {928006013, E928006013, 4},
		{928006014, E928006014, 4}, {928006015, E928006015, 4},
		{928006016, E928006016, 4}, {928006017, E928006017, 4},
		{928006018, E928006018, 4}, {928006019, E928006019, 4},
		{928006020, E928006020, 4}, {928006021, E928006021, 4},
		{928006022, E928006022, 4}, {928006023, E928006023, 4},
		{928006024, E928006024, 4}, {928007000, E928007000, 4},
		{928007001, E928007001, 4}, {928007002, E928007002, 4},
		{928007003, E928007003, 4}, {928007004, E928007004, 4},
		{928007005, E928007005, 4}, {928007006, E928007006, 4},
		{928007007, E928007007, 4}, {928007008, E928007008, 4},
		{928007009, E928007009, 4}, {928007010, E928007010, 4},
		{928007011, E928007011, 4}, {928007012, E928007012, 4},
		{928007013, E928007013, 4}, {928007014, E928007014, 4},
		{928007015, E928007015, 4}, {928007016, E928007016, 4},
		{928008000, E928008000, 4}, {928008001, E928008001, 4},
		{928008002, E928008002, 4}, {928008003, E928008003, 4},
		{928008004, E928008004, 4}, {928008005, E928008005, 4},
		{928008006, E928008006, 4}, {928008007, E928008007, 4},
		{928008008, E928008008, 4}, {928008009, E928008009, 4} };

/*** Application events ***/
const unsigned short E907000002[3]
	 = {4, 228, 228};
const unsigned short E907000003[4]
	 = {4, 4, 228, 228};
const unsigned short E907000004[4]
	 = {4, 4, 228, 228};
const unsigned short E907000011[4]
	 = {4, 4, 228, 228};
const unsigned short E907000012[4]
	 = {4, 4, 228, 228};
const unsigned short E907000014[5]
	 = {4, 4, 228, 228, 228};
const unsigned short E907000015[5]
	 = {4, 4, 228, 228, 228};
const unsigned short E907000016[4]
	 = {4, 4, 228, 228};
const unsigned short E907000018[4]
	 = {4, 4, 228, 228};
const unsigned short E907000019[4]
	 = {4, 4, 228, 228};
const unsigned short E907000020[4]
	 = {4, 4, 228, 228};
const unsigned short E907000021[4]
	 = {4, 4, 228, 228};
const unsigned short E907000022[4]
	 = {4, 4, 228, 228};
const unsigned short E907000023[5]
	 = {4, 4, 228, 228, 228};
const unsigned short E907000024[4]
	 = {4, 4, 228, 228};
const unsigned short E907003001[2]
	 = {228, 228};
const unsigned short E907003002[3]
	 = {228, 228, 228};
const unsigned short E907003003[2]
	 = {228, 228};
const unsigned short E907003004[1]
	 = {228};
const unsigned short E907003005[1]
	 = {228};
const unsigned short E907003006[2]
	 = {228, 228};
const unsigned short E907003007[3]
	 = {228, 228, 228};
const unsigned short E907003008[1]
	 = {228};
const unsigned short E907003009[1]
	 = {228};
const unsigned short E907003010[1]
	 = {228};
const unsigned short E907003011[1]
	 = {228};
const unsigned short E907003012[2]
	 = {228, 228};
const unsigned short E907003013[1]
	 = {228};
const unsigned short E907003014[2]
	 = {228, 228};
const unsigned short E907003015[3]
	 = {228, 228, 228};
const unsigned short E907006001[2]
	 = {228, 164};
const unsigned short E907006002[2]
	 = {228, 164};
const unsigned short E907006003[2]
	 = {228, 164};
const unsigned short E907006004[2]
	 = {228, 164};
const unsigned short E907006005[2]
	 = {228, 164};
const unsigned short E907006006[2]
	 = {164, 164};
const unsigned short E907006007[2]
	 = {164, 164};
const unsigned short E907006008[2]
	 = {164, 164};
const unsigned short E907006009[2]
	 = {228, 164};
const unsigned short E907006010[2]
	 = {164, 164};
const unsigned short E907006011[2]
	 = {164, 164};
const unsigned short E907006012[2]
	 = {164, 164};
const unsigned short E907006013[2]
	 = {164, 164};
const unsigned short E907006014[3]
	 = {228, 4, 164};
const unsigned short E907006015[2]
	 = {228, 164};
const unsigned short E907006016[2]
	 = {228, 164};
const unsigned short E907006017[2]
	 = {228, 164};
const unsigned short E907006018[2]
	 = {228, 164};
const unsigned short E907006019[2]
	 = {228, 164};
const unsigned short E907006020[2]
	 = {228, 164};
const unsigned short E907006021[2]
	 = {228, 164};
const unsigned short E907006022[2]
	 = {228, 164};
const unsigned short E907006023[2]
	 = {228, 164};
const unsigned short E907006024[2]
	 = {228, 164};
const unsigned short E907006025[2]
	 = {228, 164};
const unsigned short E907006026[2]
	 = {228, 164};
const unsigned short E907006027[2]
	 = {228, 164};
const unsigned short E907006028[2]
	 = {228, 164};
const unsigned short E907006029[2]
	 = {228, 164};
const unsigned short E907006030[2]
	 = {228, 164};
const unsigned short E907006031[2]
	 = {228, 164};
const unsigned short E907006032[2]
	 = {228, 164};
const unsigned short E907006033[2]
	 = {228, 164};
const unsigned short E907006034[2]
	 = {164, 228};
const unsigned short E907006035[1]
	 = {228};
const unsigned short E907006036[1]
	 = {228};
const unsigned short E907006037[3]
	 = {4, 132, 228};
const unsigned short E907006038[3]
	 = {132, 116, 228};
const unsigned short E907009001[2]
	 = {164, 228};
const unsigned short E907009002[2]
	 = {164, 228};
const unsigned short E907009003[2]
	 = {164, 228};
const unsigned short E907009004[2]
	 = {164, 228};
const unsigned short E907009005[2]
	 = {164, 228};
const unsigned short E907009006[2]
	 = {164, 228};
const unsigned short E907009007[2]
	 = {164, 228};
const unsigned short E907009008[2]
	 = {164, 228};
const unsigned short E907009009[2]
	 = {164, 228};
const unsigned short E907009010[2]
	 = {164, 228};
const unsigned short E907009011[2]
	 = {164, 228};
const unsigned short E907009012[2]
	 = {164, 228};
const unsigned short E907009013[2]
	 = {164, 228};
const unsigned short E907012001[3]
	 = {164, 228, 4};
const unsigned short E907012002[6]
	 = {164, 228, 164, 4, 164, 4};
const unsigned short E907012003[4]
	 = {164, 228, 4, 4};
const unsigned short E907012004[2]
	 = {164, 228};
const unsigned short E907012105[2]
	 = {116, 228};
const unsigned short E907012106[2]
	 = {116, 228};
const unsigned short E907012107[2]
	 = {116, 228};
const unsigned short E907015001[3]
	 = {164, 4, 228};
const unsigned short E907015600[4]
	 = {164, 4, 228, 228};
const unsigned short E907015601[4]
	 = {164, 4, 228, 228};
const unsigned short E907015700[3]
	 = {164, 4, 228};
const unsigned short E907015701[3]
	 = {164, 4, 228};
const unsigned short E907015702[4]
	 = {164, 4, 228, 228};
const unsigned short E907015703[4]
	 = {164, 4, 228, 228};
const unsigned short E907018001[1]
	 = {228};
const unsigned short E907018002[1]
	 = {4};
const unsigned short E907018003[1]
	 = {4};
const unsigned short E907018004[1]
	 = {4};
const unsigned short E907018005[2]
	 = {4, 228};
const unsigned short E907018006[2]
	 = {4, 228};
const unsigned short E907018007[2]
	 = {4, 228};
const unsigned short E907018008[1]
	 = {4};
const unsigned short E907018009[2]
	 = {4, 4};
const unsigned short E907018010[1]
	 = {4};
const unsigned short E907018011[1]
	 = {4};
const unsigned short E907021001[2]
	 = {164, 116};
const unsigned short E907021002[2]
	 = {164, 116};
const unsigned short E907021003[3]
	 = {228, 228, 228};
const unsigned short E907021004[1]
	 = {4};
const unsigned short E907021005[2]
	 = {164, 116};
const unsigned short E907024001[2]
	 = {164, 228};
const unsigned short E907024002[2]
	 = {164, 228};
const unsigned short E907024003[2]
	 = {164, 228};
const unsigned short E907024004[3]
	 = {4, 164, 228};
const unsigned short E907024005[4]
	 = {4, 132, 164, 228};
const unsigned short E907024006[5]
	 = {132, 4, 164, 4, 228};
const unsigned short E907030001[2]
	 = {164, 108};
const unsigned short E907030003[1]
	 = {164};
const unsigned short E907030004[2]
	 = {164, 108};
const unsigned short E907030005[2]
	 = {164, 104};
const unsigned short E907030007[3]
	 = {164, 164, 116};
const unsigned short E907030008[3]
	 = {228, 164, 228};
const unsigned short E907030009[4]
	 = {228, 4, 164, 228};
const unsigned short E907031002[4]
	 = {164, 164, 228, 228};
const unsigned short E907031003[6]
	 = {164, 164, 1, 228, 160, 160};
const unsigned short E907031004[3]
	 = {164, 164, 228};
const unsigned short E907031005[6]
	 = {164, 1, 1, 106, 106, 1};
const unsigned short E907031006[4]
	 = {164, 164, 228, 228};
const unsigned short E907031007[3]
	 = {164, 228, 228};
const unsigned short E907032001[3]
	 = {1, 1, 4};
const unsigned short E907033001[1]
	 = {356};
const unsigned short E907033002[3]
	 = {356, 4, 4};
const unsigned short E907033003[3]
	 = {356, 164, 228};
const unsigned short E907033004[2]
	 = {356, 228};
const unsigned short E907033005[2]
	 = {356, 104};
const unsigned short E907033006[2]
	 = {356, 164};
const unsigned short E907033007[2]
	 = {356, 228};
const unsigned short E907033008[2]
	 = {356, 4};
const unsigned short E907040001[3]
	 = {4, 228, 228};
const unsigned short E907040002[4]
	 = {4, 228, 116, 228};
const unsigned short E907040003[3]
	 = {4, 228, 228};
const unsigned short E907051001[3]
	 = {164, 228, 228};
const unsigned short E907051002[5]
	 = {164, 228, 164, 228, 228};
const unsigned short E907051003[7]
	 = {164, 228, 164, 1, 228, 228, 228};
const unsigned short E907051004[4]
	 = {164, 228, 164, 228};
const unsigned short E907051005[4]
	 = {164, 228, 164, 228};
const unsigned short E907052001[4]
	 = {4, 4, 4, 228};
const unsigned short E907052002[4]
	 = {4, 4, 4, 228};
const unsigned short E907052003[4]
	 = {4, 164, 4, 228};
const unsigned short E907052004[4]
	 = {4, 164, 4, 228};
const unsigned short E907061001[2]
	 = {164, 164};
const unsigned short E907061002[3]
	 = {164, 164, 164};
const unsigned short E907069001[2]
	 = {164, 106};
const unsigned short E907069002[3]
	 = {164, 4, 228};
const unsigned short E907069003[2]
	 = {164, 4};
const unsigned short E907069004[2]
	 = {164, 228};
const unsigned short E907069005[2]
	 = {164, 228};
const unsigned short E907069006[4]
	 = {164, 164, 228, 4};
const unsigned short E907090001[1]
	 = {164};
const unsigned short E907090002[2]
	 = {164, 164};
const unsigned short E907090003[4]
	 = {164, 164, 164, 228};
const unsigned short E907090004[2]
	 = {164, 164};
const unsigned short E907090005[2]
	 = {164, 164};
const unsigned short E907090006[4]
	 = {164, 164, 164, 228};
const unsigned short E907091001[2]
	 = {164, 228};
const unsigned short E907091002[2]
	 = {164, 228};
const unsigned short E907091003[2]
	 = {164, 228};
const unsigned short E907091004[2]
	 = {164, 228};
const unsigned short E907092001[5]
	 = {4, 4, 101, 1, 228};
const unsigned short E907100001[1]
	 = {228};
const unsigned short E907102001[1]
	 = {228};
const unsigned short E907102002[1]
	 = {228};
const unsigned short E907102003[1]
	 = {228};
const unsigned short E907102004[1]
	 = {228};
const unsigned short E907102005[1]
	 = {228};
const unsigned short E907102006[1]
	 = {228};
const unsigned short E907102007[1]
	 = {228};
const unsigned short E907137000[3]
	 = {4, 4, 4};
const unsigned short E907137001[1]
	 = {0};
const unsigned short E907137003[2]
	 = {4, 4};
const unsigned short E907119001[5]
	 = {164, 4, 6, 356, 164};
const unsigned short E907119002[6]
	 = {164, 4, 6, 132, 228, 228};
const unsigned short E907124001[2]
	 = {164, 164};
const unsigned short E907124002[2]
	 = {164, 228};
const unsigned short E907126001[5]
	 = {116, 6, 112, 228, 228};
const unsigned short E907126002[5]
	 = {116, 6, 136, 228, 228};
const unsigned short E907126003[8]
	 = {116, 6, 136, 164, 132, 132, 4, 228};
const unsigned short E907132001[4]
	 = {150, 150, 6, 356};
const unsigned short E907132002[5]
	 = {150, 1, 1, 228, 356};
const unsigned short E907142002[2]
	 = {4, 4};
const unsigned short E907144001[2]
	 = {228, 4};
const unsigned short E907144002[3]
	 = {228, 4, 164};
const unsigned short E907144003[5]
	 = {228, 4, 4, 164, 4};
const unsigned short E907145001[3]
	 = {164, 4, 4};
const unsigned short E907350000[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350001[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350002[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350003[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350004[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350005[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350006[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350007[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907350008[10]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 108, 228,
		};
const unsigned short E907350009[9]
	 = {108, 108, 108, 108, 108, 108, 108, 108, 228};
const unsigned short E907400000[4]
	 = {164, 132, 116, 228};
const unsigned short E907400001[7]
	 = {164, 132, 116, 116, 116, 164, 228};
const unsigned short E907400002[9]
	 = {164, 132, 116, 228, 4, 1, 228, 228, 228};
const unsigned short E907400003[7]
	 = {164, 132, 116, 228, 5, 228, 228};
const unsigned short E907400005[5]
	 = {164, 132, 612, 132, 228};
const unsigned short E907400006[10]
	 = {164, 132, 4, 612, 4, 612, 4, 612, 4, 228,
		};
const unsigned short E907400007[5]
	 = {164, 132, 612, 132, 228};
const unsigned short E907400008[4]
	 = {164, 132, 132, 228};
const unsigned short E907400009[4]
	 = {164, 132, 612, 228};
const unsigned short E907400010[4]
	 = {164, 132, 612, 228};
const unsigned short E907400011[4]
	 = {164, 132, 612, 228};
const unsigned short E907400012[5]
	 = {164, 132, 4, 612, 228};
const unsigned short E907400013[4]
	 = {164, 132, 612, 228};
const unsigned short E907400014[4]
	 = {164, 132, 612, 228};
const unsigned short E907400015[4]
	 = {164, 132, 612, 228};
const unsigned short E907400016[3]
	 = {164, 164, 228};
const unsigned short E907400017[7]
	 = {164, 132, 4, 4, 4, 4, 228};
const unsigned short E907400283[5]
	 = {4, 4, 101, 1, 228};
const unsigned short E907400281[4]
	 = {4, 4, 4, 228};
const unsigned short E907400282[5]
	 = {4, 4, 164, 4, 228};
const unsigned short E907403000[1]
	 = {228};
const unsigned short E907046001[3]
	 = {4, 228, 228};
const unsigned short E907046002[3]
	 = {4, 228, 228};
const unsigned short E907046003[3]
	 = {4, 228, 228};
const unsigned short E907046004[3]
	 = {4, 228, 228};
const struct imonitor_event_index Application_events[230]
	= { {907000002, E907000002, 3}, {907000003, E907000003, 4},
		{907000004, E907000004, 4}, {907000011, E907000011, 4},
		{907000012, E907000012, 4}, {907000014, E907000014, 5},
		{907000015, E907000015, 5}, {907000016, E907000016, 4},
		{907000018, E907000018, 4}, {907000019, E907000019, 4},
		{907000020, E907000020, 4}, {907000021, E907000021, 4},
		{907000022, E907000022, 4}, {907000023, E907000023, 5},
		{907000024, E907000024, 4}, {907003001, E907003001, 2},
		{907003002, E907003002, 3}, {907003003, E907003003, 2},
		{907003004, E907003004, 1}, {907003005, E907003005, 1},
		{907003006, E907003006, 2}, {907003007, E907003007, 3},
		{907003008, E907003008, 1}, {907003009, E907003009, 1},
		{907003010, E907003010, 1}, {907003011, E907003011, 1},
		{907003012, E907003012, 2}, {907003013, E907003013, 1},
		{907003014, E907003014, 2}, {907003015, E907003015, 3},
		{907006001, E907006001, 2}, {907006002, E907006002, 2},
		{907006003, E907006003, 2}, {907006004, E907006004, 2},
		{907006005, E907006005, 2}, {907006006, E907006006, 2},
		{907006007, E907006007, 2}, {907006008, E907006008, 2},
		{907006009, E907006009, 2}, {907006010, E907006010, 2},
		{907006011, E907006011, 2}, {907006012, E907006012, 2},
		{907006013, E907006013, 2}, {907006014, E907006014, 3},
		{907006015, E907006015, 2}, {907006016, E907006016, 2},
		{907006017, E907006017, 2}, {907006018, E907006018, 2},
		{907006019, E907006019, 2}, {907006020, E907006020, 2},
		{907006021, E907006021, 2}, {907006022, E907006022, 2},
		{907006023, E907006023, 2}, {907006024, E907006024, 2},
		{907006025, E907006025, 2}, {907006026, E907006026, 2},
		{907006027, E907006027, 2}, {907006028, E907006028, 2},
		{907006029, E907006029, 2}, {907006030, E907006030, 2},
		{907006031, E907006031, 2}, {907006032, E907006032, 2},
		{907006033, E907006033, 2}, {907006034, E907006034, 2},
		{907006035, E907006035, 1}, {907006036, E907006036, 1},
		{907006037, E907006037, 3}, {907006038, E907006038, 3},
		{907009001, E907009001, 2}, {907009002, E907009002, 2},
		{907009003, E907009003, 2}, {907009004, E907009004, 2},
		{907009005, E907009005, 2}, {907009006, E907009006, 2},
		{907009007, E907009007, 2}, {907009008, E907009008, 2},
		{907009009, E907009009, 2}, {907009010, E907009010, 2},
		{907009011, E907009011, 2}, {907009012, E907009012, 2},
		{907009013, E907009013, 2}, {907012001, E907012001, 3},
		{907012002, E907012002, 6}, {907012003, E907012003, 4},
		{907012004, E907012004, 2}, {907012105, E907012105, 2},
		{907012106, E907012106, 2}, {907012107, E907012107, 2},
		{907015001, E907015001, 3}, {907015600, E907015600, 4},
		{907015601, E907015601, 4}, {907015700, E907015700, 3},
		{907015701, E907015701, 3}, {907015702, E907015702, 4},
		{907015703, E907015703, 4}, {907018001, E907018001, 1},
		{907018002, E907018002, 1}, {907018003, E907018003, 1},
		{907018004, E907018004, 1}, {907018005, E907018005, 2},
		{907018006, E907018006, 2}, {907018007, E907018007, 2},
		{907018008, E907018008, 1}, {907018009, E907018009, 2},
		{907018010, E907018010, 1}, {907018011, E907018011, 1},
		{907021001, E907021001, 2}, {907021002, E907021002, 2},
		{907021003, E907021003, 3}, {907021004, E907021004, 1},
		{907021005, E907021005, 2}, {907024001, E907024001, 2},
		{907024002, E907024002, 2}, {907024003, E907024003, 2},
		{907024004, E907024004, 3}, {907024005, E907024005, 4},
		{907024006, E907024006, 5}, {907030001, E907030001, 2},
		{907030003, E907030003, 1}, {907030004, E907030004, 2},
		{907030005, E907030005, 2}, {907030007, E907030007, 3},
		{907030008, E907030008, 3}, {907030009, E907030009, 4},
		{907031002, E907031002, 4}, {907031003, E907031003, 6},
		{907031004, E907031004, 3}, {907031005, E907031005, 6},
		{907031006, E907031006, 4}, {907031007, E907031007, 3},
		{907032001, E907032001, 3}, {907033001, E907033001, 1},
		{907033002, E907033002, 3}, {907033003, E907033003, 3},
		{907033004, E907033004, 2}, {907033005, E907033005, 2},
		{907033006, E907033006, 2}, {907033007, E907033007, 2},
		{907033008, E907033008, 2}, {907040001, E907040001, 3},
		{907040002, E907040002, 4}, {907040003, E907040003, 3},
		{907051001, E907051001, 3}, {907051002, E907051002, 5},
		{907051003, E907051003, 7}, {907051004, E907051004, 4},
		{907051005, E907051005, 4}, {907052001, E907052001, 4},
		{907052002, E907052002, 4}, {907052003, E907052003, 4},
		{907052004, E907052004, 4}, {907061001, E907061001, 2},
		{907061002, E907061002, 3}, {907069001, E907069001, 2},
		{907069002, E907069002, 3}, {907069003, E907069003, 2},
		{907069004, E907069004, 2}, {907069005, E907069005, 2},
		{907069006, E907069006, 4}, {907090001, E907090001, 1},
		{907090002, E907090002, 2}, {907090003, E907090003, 4},
		{907090004, E907090004, 2}, {907090005, E907090005, 2},
		{907090006, E907090006, 4}, {907091001, E907091001, 2},
		{907091002, E907091002, 2}, {907091003, E907091003, 2},
		{907091004, E907091004, 2}, {907092001, E907092001, 5},
		{907100001, E907100001, 1}, {907102001, E907102001, 1},
		{907102002, E907102002, 1}, {907102003, E907102003, 1},
		{907102004, E907102004, 1}, {907102005, E907102005, 1},
		{907102006, E907102006, 1}, {907102007, E907102007, 1},
		{907137000, E907137000, 3}, {907137001, E907137001, 0},
		{907137003, E907137003, 2}, {907119001, E907119001, 5},
		{907119002, E907119002, 6}, {907124001, E907124001, 2},
		{907124002, E907124002, 2}, {907126001, E907126001, 5},
		{907126002, E907126002, 5}, {907126003, E907126003, 8},
		{907132001, E907132001, 4}, {907132002, E907132002, 5},
		{907142002, E907142002, 2}, {907144001, E907144001, 2},
		{907144002, E907144002, 3}, {907144003, E907144003, 5},
		{907145001, E907145001, 3}, {907350000, E907350000, 9},
		{907350001, E907350001, 9}, {907350002, E907350002, 9},
		{907350003, E907350003, 9}, {907350004, E907350004, 9},
		{907350005, E907350005, 9}, {907350006, E907350006, 9},
		{907350007, E907350007, 9}, {907350008, E907350008, 10},
		{907350009, E907350009, 9}, {907400000, E907400000, 4},
		{907400001, E907400001, 7}, {907400002, E907400002, 9},
		{907400003, E907400003, 7}, {907400005, E907400005, 5},
		{907400006, E907400006, 10}, {907400007, E907400007, 5},
		{907400008, E907400008, 4}, {907400009, E907400009, 4},
		{907400010, E907400010, 4}, {907400011, E907400011, 4},
		{907400012, E907400012, 5}, {907400013, E907400013, 4},
		{907400014, E907400014, 4}, {907400015, E907400015, 4},
		{907400016, E907400016, 3}, {907400017, E907400017, 7},
		{907400283, E907400283, 5}, {907400281, E907400281, 4},
		{907400282, E907400282, 5}, {907403000, E907403000, 1},
		{907046001, E907046001, 3}, {907046002, E907046002, 3},
		{907046003, E907046003, 3}, {907046004, E907046004, 3} };

/*** LogEngine events ***/
const unsigned short E908000000[3]
	 = {356, 4, 4};
const unsigned short E908000001[4]
	 = {356, 4, 4, 4};
const unsigned short E908000002[2]
	 = {356, 4};
const unsigned short E908000003[1]
	 = {356};
const unsigned short E908000004[2]
	 = {4, 612};
const unsigned short E908000005[1]
	 = {0};
const unsigned short E908001001[1]
	 = {2};
const unsigned short E908001002[1]
	 = {2};
const unsigned short E908001003[1]
	 = {2};
const unsigned short E908001004[1]
	 = {2};
const unsigned short E908001005[1]
	 = {0};
const unsigned short E908002001[4]
	 = {125, 125, 125, 125};
const struct imonitor_event_index LogEngine_events[12]
	= { {908000000, E908000000, 3}, {908000001, E908000001, 4},
		{908000002, E908000002, 2}, {908000003, E908000003, 1},
		{908000004, E908000004, 2}, {908000005, E908000005, 0},
		{908001001, E908001001, 1}, {908001002, E908001002, 1},
		{908001003, E908001003, 1}, {908001004, E908001004, 1},
		{908001005, E908001005, 0}, {908002001, E908002001, 4} };

/*** Wifi events ***/
const unsigned short E909001001[16]
	 = {4, 4, 4, 4, 4, 4, 1, 4, 1, 4,
		1, 4, 2, 4, 2, 4};
const unsigned short E909001002[34]
	 = {132, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 3,
		3, 2, 2, 4};
const unsigned short E909002001[3]
	 = {2, 2, 1};
const unsigned short E909002002[6]
	 = {2, 2, 1, 1, 1, 1};
const unsigned short E909002003[36]
	 = {2, 2, 116, 117, 132, 220, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 2, 3, 3, 2,
		2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
		2, 2, 2, 2, 2, 4};
const unsigned short E909002004[29]
	 = {2, 2, 117, 132, 220, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 2, 3, 3, 2, 2, 2,
		2, 2, 2, 2, 2, 1, 1, 1, 1};
const unsigned short E909002005[29]
	 = {2, 2, 117, 132, 220, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 2, 3, 3, 2, 2, 2,
		2, 2, 2, 2, 2, 1, 1, 1, 1};
const unsigned short E909002006[31]
	 = {2, 2, 117, 132, 220, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 2, 3, 3, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
		1};
const unsigned short E909002007[113]
	 = {2, 2, 117, 132, 220, 2, 2, 2, 3, 3,
		3, 3, 3, 3, 3, 3, 2, 2, 2, 2,
		2, 2, 2, 2, 3, 3, 1, 2, 1, 1,
		1, 2, 115, 2, 115, 131, 4, 2, 2, 102,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 1, 500,
		224, 1, 1};
const unsigned short E909202007[114]
	 = {2, 2, 117, 132, 220, 2, 2, 2, 3, 3,
		3, 3, 3, 3, 3, 3, 2, 2, 2, 2,
		2, 2, 2, 2, 3, 3, 1, 2, 1, 1,
		1, 2, 115, 2, 115, 131, 4, 2, 2, 148,
		102, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 1,
		500, 224, 1, 1};
const unsigned short E909002008[122]
	 = {2, 2, 117, 132, 220, 2, 2, 2, 3, 3,
		3, 3, 3, 3, 3, 3, 2, 2, 2, 2,
		2, 2, 2, 2, 3, 3, 1, 2, 1, 1,
		1, 2, 115, 2, 115, 131, 4, 2, 2, 102,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 1, 4,
		4, 4, 4, 4, 4, 4, 1, 4, 4, 4,
		4, 1};
const unsigned short E909202008[123]
	 = {2, 2, 117, 132, 220, 2, 2, 2, 3, 3,
		3, 3, 3, 3, 3, 3, 2, 2, 2, 2,
		2, 2, 2, 2, 3, 3, 1, 2, 1, 1,
		1, 2, 115, 2, 115, 131, 4, 2, 2, 148,
		102, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 1,
		4, 4, 4, 4, 4, 4, 4, 1, 4, 4,
		4, 4, 1};
const unsigned short E909002009[28]
	 = {2, 2, 117, 132, 220, 3, 3, 2, 3, 3,
		1, 1, 1, 1, 2, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4};
const unsigned short E909002010[28]
	 = {2, 2, 117, 132, 220, 3, 3, 2, 3, 3,
		1, 1, 1, 1, 2, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4};
const unsigned short E909030001[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030002[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030003[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030004[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030005[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030006[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030007[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030008[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030009[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030010[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030011[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030012[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030013[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030014[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030015[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030016[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030017[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030018[4]
	 = {4, 164, 164, 1124};
const unsigned short E909030019[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031001[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031002[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031003[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031004[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031005[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031006[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031007[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031008[4]
	 = {4, 164, 164, 1124};
const unsigned short E909031009[4]
	 = {4, 164, 164, 1124};
const unsigned short E909800001[2]
	 = {1124, 4};
const unsigned short E909800002[2]
	 = {1124, 4};
const unsigned short E909800003[2]
	 = {1124, 4};
const unsigned short E909800004[2]
	 = {1124, 4};
const struct imonitor_event_index Wifi_events[46]
	= { {909001001, E909001001, 16}, {909001002, E909001002, 34},
		{909002001, E909002001, 3}, {909002002, E909002002, 6},
		{909002003, E909002003, 36}, {909002004, E909002004, 29},
		{909002005, E909002005, 29}, {909002006, E909002006, 31},
		{909002007, E909002007, 113}, {909202007, E909202007, 114},
		{909002008, E909002008, 122}, {909202008, E909202008, 123},
		{909002009, E909002009, 28}, {909002010, E909002010, 28},
		{909030001, E909030001, 4}, {909030002, E909030002, 4},
		{909030003, E909030003, 4}, {909030004, E909030004, 4},
		{909030005, E909030005, 4}, {909030006, E909030006, 4},
		{909030007, E909030007, 4}, {909030008, E909030008, 4},
		{909030009, E909030009, 4}, {909030010, E909030010, 4},
		{909030011, E909030011, 4}, {909030012, E909030012, 4},
		{909030013, E909030013, 4}, {909030014, E909030014, 4},
		{909030015, E909030015, 4}, {909030016, E909030016, 4},
		{909030017, E909030017, 4}, {909030018, E909030018, 4},
		{909030019, E909030019, 4}, {909031001, E909031001, 4},
		{909031002, E909031002, 4}, {909031003, E909031003, 4},
		{909031004, E909031004, 4}, {909031005, E909031005, 4},
		{909031006, E909031006, 4}, {909031007, E909031007, 4},
		{909031008, E909031008, 4}, {909031009, E909031009, 4},
		{909800001, E909800001, 2}, {909800002, E909800002, 2},
		{909800003, E909800003, 2}, {909800004, E909800004, 2} };

/*** GPS events ***/
const unsigned short E910000000[23]
	 = {150, 108, 4, 2, 2, 2, 4, 4, 4, 4,
		4, 2, 2, 2, 4, 4, 4, 1, 1, 4,
		4, 2, 4};
const unsigned short E910000001[43]
	 = {4, 3, 2, 1, 1, 103, 103, 4, 4, 4,
		117, 132, 2, 106, 4, 4, 4, 4, 4, 324,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 4, 2, 1,
		2, 2, 300};
const unsigned short E910000002[11]
	 = {3, 3, 3, 3, 3, 3, 3, 3, 1, 1,
		3};
const unsigned short E910800001[2]
	 = {1124, 4};
const unsigned short E910800002[2]
	 = {1124, 4};
const struct imonitor_event_index GPS_events[5]
	= { {910000000, E910000000, 23}, {910000001, E910000001, 43},
		{910000002, E910000002, 11}, {910800001, E910800001, 2},
		{910800002, E910800002, 2} };

/*** Fingerprint events ***/
const unsigned short E912000000[24]
	 = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 132, 132, 4,
		132, 4, 4, 4};
const unsigned short E912000001[24]
	 = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 132, 132, 4,
		132, 4, 4, 4};
const unsigned short E912000002[13]
	 = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 132, 132};
const unsigned short E912000003[19]
	 = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 132, 132};
const unsigned short E912001000[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001001[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001002[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001003[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001004[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001005[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001006[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001007[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001008[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001009[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001010[4]
	 = {4, 164, 164, 1124};
const unsigned short E912001011[4]
	 = {4, 164, 164, 1124};
const struct imonitor_event_index Fingerprint_events[16]
	= { {912000000, E912000000, 24}, {912000001, E912000001, 24},
		{912000002, E912000002, 13}, {912000003, E912000003, 19},
		{912001000, E912001000, 4}, {912001001, E912001001, 4},
		{912001002, E912001002, 4}, {912001003, E912001003, 4},
		{912001004, E912001004, 4}, {912001005, E912001005, 4},
		{912001006, E912001006, 4}, {912001007, E912001007, 4},
		{912001008, E912001008, 4}, {912001009, E912001009, 4},
		{912001010, E912001010, 4}, {912001011, E912001011, 4} };

/*** BT events ***/
const unsigned short E913000000[21]
	 = {4, 4, 120, 117, 4, 4, 4, 106, 4, 4,
		4, 4, 120, 150, 130, 130, 125, 105, 105, 105,
		105};
const unsigned short E913000001[51]
	 = {4, 4, 120, 117, 4, 4, 4, 106, 4, 4,
		4, 4, 4, 4, 5, 5, 5, 5, 4, 5,
		5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 120, 150, 130, 130, 125,
		130, 135, 135, 135, 135, 135, 130, 105, 130, 130,
		130};
const unsigned short E913000002[51]
	 = {4, 4, 120, 117, 4, 4, 4, 106, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 116, 116, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 120, 150, 130, 130, 125, 105, 105, 105,
		105, 105, 105, 132, 105, 105, 105, 125, 125, 105,
		105};
const unsigned short E913001000[13]
	 = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4};
const unsigned short E913001001[47]
	 = {4, 4, 117, 120, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 120, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		116, 4, 4, 110, 130, 130, 106};
const unsigned short E913002001[4]
	 = {4, 164, 164, 1124};
const unsigned short E913002002[4]
	 = {4, 164, 164, 1124};
const unsigned short E913002003[4]
	 = {4, 164, 164, 1124};
const unsigned short E913002004[4]
	 = {4, 164, 164, 1124};
const unsigned short E913002005[4]
	 = {4, 164, 164, 1124};
const unsigned short E913002006[4]
	 = {4, 164, 164, 1124};
const unsigned short E913800001[2]
	 = {1124, 4};
const unsigned short E913800002[2]
	 = {1124, 4};
const struct imonitor_event_index BT_events[13]
	= { {913000000, E913000000, 21}, {913000001, E913000001, 51},
		{913000002, E913000002, 51}, {913001000, E913001000, 13},
		{913001001, E913001001, 47}, {913002001, E913002001, 4},
		{913002002, E913002002, 4}, {913002003, E913002003, 4},
		{913002004, E913002004, 4}, {913002005, E913002005, 4},
		{913002006, E913002006, 4}, {913800001, E913800001, 2},
		{913800002, E913800002, 2} };

/*** FileSystem events ***/
const unsigned short E914000000[8]
	 = {4, 4, 4, 4, 5, 4, 4, 5};
const unsigned short E914001000[6]
	 = {117, 4, 4, 4, 4, 4};
const unsigned short E914002000[5]
	 = {117, 4, 4, 4, 4};
const unsigned short E914003000[5]
	 = {117, 4, 4, 4, 4};
const struct imonitor_event_index FileSystem_events[4]
	= { {914000000, E914000000, 8}, {914001000, E914001000, 6},
		{914002000, E914002000, 5}, {914003000, E914003000, 5} };

/*** NFC events ***/
const unsigned short E915800001[2]
	 = {1124, 4};
const unsigned short E915800002[2]
	 = {1124, 4};
const struct imonitor_event_index NFC_events[2]
	= { {915800001, E915800001, 2}, {915800002, E915800002, 2} };

/*** Audio events ***/
const unsigned short E916000001[2]
	 = {4, 164};
const unsigned short E916000002[2]
	 = {4, 164};
const unsigned short E916000003[2]
	 = {4, 164};
const unsigned short E916000004[3]
	 = {4, 164, 2};
const unsigned short E916000005[3]
	 = {4, 164, 164};
const unsigned short E916000101[9]
	 = {4, 164, 4, 4, 4, 4, 4, 4, 4};
const unsigned short E916000102[9]
	 = {4, 164, 4, 4, 4, 4, 4, 4, 4};
const unsigned short E916000103[9]
	 = {4, 164, 4, 4, 4, 4, 4, 4, 4};
const unsigned short E916000104[9]
	 = {4, 164, 4, 4, 4, 4, 4, 4, 4};
const struct imonitor_event_index Audio_events[9]
	= { {916000001, E916000001, 2}, {916000002, E916000002, 2},
		{916000003, E916000003, 2}, {916000004, E916000004, 3},
		{916000005, E916000005, 3}, {916000101, E916000101, 9},
		{916000102, E916000102, 9}, {916000103, E916000103, 9},
		{916000104, E916000104, 9} };

/* All events index table */
const unsigned int imonitor_modules_count = 14;
const struct imonitor_module_index imonitor_modules_table[14] = {
	{Reliability_events, 901000000, 901999999, 20},
	{Performance_events, 902000000, 902999999, 41},
	{PowerThermal_events, 903000000, 903999999, 36},
	{CHR_events, 904000000, 904999999, 13},
	{DMD_events, 920000000, 939999999, 620},
	{Application_events, 907000000, 907999999, 230},
	{LogEngine_events, 908000000, 908999999, 12},
	{Wifi_events, 909000000, 909999999, 46},
	{GPS_events, 910000000, 910999999, 5},
	{Fingerprint_events, 912000000, 912999999, 16},
	{BT_events, 913000000, 913999999, 13},
	{FileSystem_events, 914000000, 914999999, 4},
	{NFC_events, 915000000, 915999999, 2},
	{Audio_events, 916000000, 916999999, 9},
};

#ifdef __cplusplus
}
#endif
#endif /* _IMONITOR_EVENTS_H_ */
