<script setup lang="ts">
import { Ref, ref, watch } from 'vue';

import { Info, DateTime } from 'luxon';

import { useI18n } from 'vue-i18n';

import componentI18n from './CalendarComponentI18n';

const props = defineProps(['yearMonthDay']);

const emit = defineEmits(['daySelected']);

const { t } = useI18n(componentI18n);

enum CalendarViewType {
    MONTH,
    YEAR
}

type WeekdayShortNames = Array<string>;

type Weekdays = {
    days: Array<DateTime>,
    weekNumber: number
}

type Month = {
    monthIndex: number
    monthLong: string,
    year: number
}

type MonthCalendar = {
    month: Month,
    weeks: Array<Weekdays>
}

function newMonthCalendar(year: number, monthIndex: number, monthLong: string | null): MonthCalendar {
    return {
        month: {
        monthIndex: monthIndex,
        monthLong: monthLong || '',
        year: year
    },
    weeks: []
  };
}

type YearCalendar = {
    months: Array<Month>,
    year: number
}

function newYearCalendar(year: number): YearCalendar {
    const yearCalendar: YearCalendar = {
        months: [],
        year: year
    }

    const monthNames: string[] = Info.months('long', { locale });
    for (let i = 0; i < monthNames.length; i++) {
        yearCalendar.months[i] = {
            monthIndex: i + 1,
            monthLong: monthNames[i],
            year: year
        }
    }

  return yearCalendar;
}

const locale: string = 'fr-FR';

const today: DateTime = DateTime.now();

const calendarViewType: Ref<CalendarViewType> = ref(CalendarViewType.MONTH);

const selectedDate: Ref<DateTime> = ref(today);
if (props.yearMonthDay !== null) {
    selectedDate.value = DateTime.fromISO(props.yearMonthDay);
}

const weekdays: Ref<WeekdayShortNames> = ref(Info.weekdays('short', { locale }));

const monthCalendar: Ref<MonthCalendar> = ref(newMonthCalendar(today.year, today.month, today.monthLong));
const yearCalendar: Ref<YearCalendar> = ref(newYearCalendar(today.year));

function buildMonthCalendar(year: number, month: number):  MonthCalendar {
    let dateInMonth: DateTime = DateTime.local(year, month, 1, { locale });

    const calendar: MonthCalendar = newMonthCalendar(year, month, dateInMonth.monthLong);

    let currentWeek: Weekdays = {
        days: new Array(7).fill(null),
        weekNumber: dateInMonth.weekNumber
    }

    const daysMonth: number = dateInMonth.daysInMonth || 0;
    for (let day = 1; day <= daysMonth; day++) {
        const weekdayIndex = (dateInMonth.localWeekday-1) % 7;
        currentWeek.days[weekdayIndex] = dateInMonth;
        if (weekdayIndex === 6 || day === daysMonth) {
            calendar.weeks.push(currentWeek);
            currentWeek = {
                days: new Array(7).fill(null),
                weekNumber: 0
            }
        }
        dateInMonth = dateInMonth.plus({ days: 1 });
        currentWeek.weekNumber = dateInMonth.weekNumber;
    }

    return calendar;
}

function buildYearCalendar(year: number): YearCalendar {
    return newYearCalendar(year);
}

function isToday(date: DateTime | null): boolean {
    return date?.year === today.year && date?.month === today.month && date?.day === today.day;
}

function isSelectedDay(date: DateTime | null): boolean {
    return date?.year === selectedDate.value.year && date?.month === selectedDate.value.month && date?.day === selectedDate.value.day;
}

function isNextOrPreviousMonth(date: DateTime | null): boolean {
    return date === null;
}

function onDaySelected(date: DateTime) {
    emit('daySelected', date.toFormat('yyyyMMdd'));
}

function onMonthSelected(month: Month) {
    monthCalendar.value = buildMonthCalendar(month.year, month.monthIndex);
    calendarViewType.value = CalendarViewType.MONTH;
}

function onGoToPreviousPeriod() {
    if (calendarViewType.value === CalendarViewType.MONTH) {
        let year = monthCalendar.value.month.year;
        let month = monthCalendar.value.month.monthIndex - 1;
        if (month === 0) {
            month = 12;
            year -= 1;
        }
        monthCalendar.value = buildMonthCalendar(year, month);
    } else if (calendarViewType.value === CalendarViewType.YEAR) {
        let year = yearCalendar.value.year;
        year -= 1;
        yearCalendar.value = buildYearCalendar(year);
    }
}

function onGoToNextPeriod() {
    if (calendarViewType.value === CalendarViewType.MONTH) {
        let year = monthCalendar.value.month.year;
        let month = monthCalendar.value.month.monthIndex + 1;
        if (month === 13) {
            month = 1;
            year += 1;
        }
        monthCalendar.value = buildMonthCalendar(year, month);
    } else if (calendarViewType.value === CalendarViewType.YEAR) {
        let year = yearCalendar.value.year;
        year += 1;
        yearCalendar.value = buildYearCalendar(year);
    }
}

function goToToday() {
    calendarViewType.value = CalendarViewType.MONTH;
    monthCalendar.value = buildMonthCalendar(today.year, today.month);
    yearCalendar.value = buildYearCalendar(today.year);
    onDaySelected(today);
}

function onMonthClick() {
    calendarViewType.value = CalendarViewType.YEAR;
    yearCalendar.value = buildYearCalendar(monthCalendar.value.month.year);
}

function onYearClick(year: number) {
    calendarViewType.value = CalendarViewType.YEAR;
    yearCalendar.value = buildYearCalendar(year);
}

function toShortYearString(year: number): string {
    const value: string = '' + year;
    return value.substring(2, 4);
}

watch(() => props.yearMonthDay, (value) => {
    selectedDate.value = DateTime.fromISO(value).setLocale(locale);
});

monthCalendar.value = buildMonthCalendar(selectedDate.value.year, selectedDate.value.month);
yearCalendar.value = buildYearCalendar(selectedDate.value.year);

</script>

<template>
    <div class="cl-calendar-container">
        <div class="cl-calendar-control-pane cl-calendar-header">
            <div class="cl-calendar-header-nav-left">
                <button @click="onGoToPreviousPeriod()">
                    <font-awesome-icon icon="fa-solid fa-chevron-left" class="icon" />
                </button>
            </div>
            <div v-if="calendarViewType == CalendarViewType.MONTH" class="cl-calendar-header-nav-month">
                <button @click="onMonthClick()" style="font-weight: bold;">
                    <span>{{ monthCalendar.month.monthLong }}</span>
                </button>
            </div>
            <div v-if="calendarViewType == CalendarViewType.MONTH" class="cl-calendar-header-nav-year">
                <button @click="onYearClick(monthCalendar.month.year)" style="font-weight: bold;">
                    <span>{{ monthCalendar.month.year }}</span>
                </button>
            </div>
            <div v-if="calendarViewType == CalendarViewType.YEAR" class="cl-calendar-header-nav-year">
                <button @click="onYearClick(yearCalendar.year - 2)" class="cl-calendar-if-large" style="font-size: 0.9em;">
                    <span>{{ toShortYearString(yearCalendar.year - 2) }}</span>
                </button>
                <button @click="onYearClick(yearCalendar.year - 1)" style="font-size: 1em;">
                    <span>{{ toShortYearString(yearCalendar.year - 1) }}</span>
                </button>
                <button style="font-weight: bold;">
                    <span>{{ yearCalendar.year }}</span>
                </button>
                <button @click="onYearClick(yearCalendar.year + 1)" style="font-size: 1em;">
                    <span>{{ toShortYearString(yearCalendar.year + 1) }}</span>
                </button>
                <button @click="onYearClick(yearCalendar.year + 2)" class="cl-calendar-if-large" style="font-size: 0.9em;">
                    <span>{{ toShortYearString(yearCalendar.year + 2) }}</span>
                </button>
            </div>
            <div class="cl-calendar-header-nav-right">
                <button @click="onGoToNextPeriod()">
                    <font-awesome-icon icon="fa-solid fa-chevron-right" class="icon" />
                </button>
            </div>
        </div>
        <div class="cl-calendar-body">
            <table v-if="calendarViewType == CalendarViewType.MONTH">
                <thead>
                    <tr>
                        <th></th>
                        <th v-for="weekday in weekdays" class="cl-calendar-day-name-cell">
                            {{ weekday }}
                        </th>
                    </tr>
                </thead>
                <tbody>
                    <tr v-for="week in monthCalendar.weeks">
                        <td class="cl-calendar-week-number-cell">s{{ week.weekNumber }}</td>
                        <td v-for="date in week.days" class="cl-calender-body-clickable-cell cl-calendar-day-cell" :class="{
                            'is-today': isToday(date),
                            'is-selected-calendar-item': isSelectedDay(date),
                            'is-next-or-previous-month': isNextOrPreviousMonth(date)
                        }">
                            <button v-if="date" @click="onDaySelected(date)">{{ date.day }}</button>
                        </td>
                    </tr>
                </tbody>
            </table>
            <table v-if="calendarViewType == CalendarViewType.YEAR" class="cl-calendar-body-year">
                <tr v-for="i in 4" :key="i">
                    <td v-for="month in yearCalendar.months.slice((i - 1) * 3, i * 3)" :key="month.monthIndex"
                        class="cl-calender-body-clickable-cell cl-calendar-month-cell" :class="{
                            'is-selected-calendar-item': monthCalendar.month.year === month.year && monthCalendar.month.monthIndex === month.monthIndex
                        }">
                        <button @click="onMonthSelected(month)">
                            {{ month.monthLong }}
                        </button>
                    </td>
                </tr>
            </table>
        </div>
        <div class="cl-calendar-control-pane cl-calendar-footer">
            <button @click="goToToday()">
                <font-awesome-icon icon="fa-solid fa-location-pin" class="icon" />&nbsp;{{ t('today') }}
            </button>
        </div>
    </div>
</template>

<style scoped>
.cl-calendar-container {
    --calendar-control-pane-bg-color: #3b78b0;
    --calendar-control-pane-bg-color--darker: #006ba2;
    --calendar-control-pane-text-color: white;
    min-width: 342px;
    max-width: 386px;
}

.cl-calendar-container button:hover {
    cursor: pointer;
}

.cl-calendar-control-pane {
    height: 3em;
    background-color: var(--calendar-control-pane-bg-color);
}

.cl-calendar-control-pane button:hover {
    filter: brightness(120%);
}

.cl-calendar-header {
    display: flex;
    flex-flow: row nowrap;
    justify-content: space-between;
    align-items: center;
    border-top-width: 1px;
    border-top-style: solid;
    border-top-color: var(--calendar-control-pane-bg-color);
    border-bottom-width: 1px;
    border-bottom-style: solid;
    border-bottom-color: var(--calendar-control-pane-bg-color--darker);
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}

.cl-calendar-header button {
    height: 2em;
    border-width: 1px;
    border-style: solid;
    border-color: var(--calendar-control-pane-bg-color);
    border-radius: 2em;
    background-color: var(--calendar-control-pane-bg-color);
    color: var(--calendar-control-pane-text-color);
    font-size: 1.1em;
}

.cl-calendar-header-nav-left {
    padding-left: 0.5em;
}

.cl-calendar-header-nav-left button {
    width: 2em;
}

.cl-calendar-header-nav-right {
    padding-right: 0.5em;
}

.cl-calendar-header-nav-right button {
    width: 2em;
}

.cl-calendar-header-nav-month button {
    padding-left: 0.8em;
    padding-right: 0.8em;
}

.cl-calendar-header-nav-month span {
    text-transform: capitalize;
}

.cl-calendar-header-nav-year button {
    padding-left: 0.8em;
    padding-right: 0.8em;
}

.cl-calendar-body {
    --calendar-body-bg-color: white;
    --calendar-body-item-hover-color: rgb(233, 237, 240);
    --calendar-body-font-size: 1em;
    background-color: var(--calendar-body-bg-color);
    border-left-width: 1px;
    border-left-style: solid;
    border-left-color: var(--calendar-body-bg-color);
    border-right-width: 1px;
    border-right-style: solid;
    border-right-color: var(--calendar-body-bg-color);
}

.cl-calendar-body table {
    width: 100%;
    border-spacing: 0px;
}

.cl-calendar-body-year {
    padding-top: 0.8em;
    padding-bottom: 0.8em;
}

.cl-calender-body-clickable-cell button {
    font-size: var(--calendar-body-font-size);
    color: #003f7d;
    background-color: var(--calendar-body-bg-color);
    border: 1px solid var(--calendar-body-bg-color);
}

.cl-calendar-day-name-cell {
    color: #3b78b0;
}

.cl-calendar-week-number-cell {
    height: 2.6em;
    text-align: center;
    font-size: 0.95em;
    font-weight: bold;
    color: #6c6c6c;
}

.cl-calendar-day-cell {
    text-align: center;
    font-size: 0.9em;
}

.cl-calendar-month-cell {
    width: 33%;
    text-align: center;
    padding-top: 0.4em;
    padding-bottom: 0.4em;
}

.cl-calendar-day-cell.is-next-or-previous-month {
    background-color: var(--calendar-body-item-hover-color);
    border-top-width: 0.8em;
    border-top-style: solid;
    border-top-color: var(--calendar-body-bg-color);
    border-bottom-width: 0.8em;
    border-bottom-style: solid;
    border-bottom-color: var(--calendar-body-bg-color);
}

.cl-calendar-day-cell.is-next-or-previous-month:last-of-type {
    border-right-width: 0.8em;
    border-right-style: solid;
    border-right-color: var(--calendar-body-bg-color);
}

.cl-calendar-day-cell button {
    width: 2.2em;
    height: 2.2em;
    border-radius: 50%;
}

.cl-calendar-month-cell button {
    height: 2.2em;
    width: 100%;
    border-radius: 0.8em;
    text-transform: capitalize;
}

.cl-calendar-day-cell.is-today button {
    text-decoration: underline;
    font-weight: bold;
}

.cl-calendar-day-cell.is-selected-calendar-item button {
    color: var(--calendar-body-item-hover-color);
    font-weight: bold;
    border: 1px solid #006ba2;
    border-radius: 50%;
    background-color: #006ba2;
    box-shadow: 0 0 1px 0 #006ba2 inset, 0 0 1px 0 #006ba2;
}

.cl-calendar-day-cell:not(.is-selected-calendar-item) button:hover {
    border: 1px solid var(--calendar-body-item-hover-color);
    border-radius: 50%;
    background-color: var(--calendar-body-item-hover-color);
    box-shadow: 0 0 1px 0 var(--calendar-body-item-hover-color) inset, 0 0 1px 0 var(--calendar-body-item-hover-color);
}

.cl-calendar-month-cell.is-selected-calendar-item button {
    cursor: pointer;
    color: var(--calendar-body-item-hover-color);
    font-weight: bold;
    border: 1px solid #006ba2;
    border-radius: 0.8em;
    background-color: #006ba2;
    box-shadow: 0 0 1px 0 #006ba2 inset, 0 0 1px 0 #006ba2;
}

.cl-calendar-month-cell:not(.is-selected-calendar-item) button:hover {
    border: 1px solid var(--calendar-body-item-hover-color);
    border-radius: 0.8em;
    background-color: var(--calendar-body-item-hover-color);
    box-shadow: 0 0 1px 0 var(--calendar-body-item-hover-color) inset, 0 0 1px 0 var(--calendar-body-item-hover-color);
}

.cl-calendar-footer {
    display: flex;
    flex-flow: row nowrap;
    justify-content: center;
    align-items: center;
    border-top-width: 1px;
    border-top-style: solid;
    border-top-color: var(--calendar-control-pane-bg-color--darker);
    border-bottom-width: 1px;
    border-bottom-style: solid;
    border-bottom-color: var(--calendar-control-pane-bg-color);
    border-bottom-left-radius: 6px;
    border-bottom-right-radius: 6px;
}

.cl-calendar-footer button {
    height: 2em;
    padding-left: 1em;
    padding-right: 1em;
    border-width: 1px;
    border-style: solid;
    border-color: var(--calendar-control-pane-bg-color);
    border-radius: 2em;
    background-color: var(--calendar-control-pane-bg-color);
    color: var(--calendar-control-pane-text-color);
    font-size: 1em;
    font-weight: bold;
}

@media screen and (min-width: 362px) {
    .cl-calendar-if-large {
        display: inline;
    }
}

@media screen and (max-width: 361px) {
    .cl-calendar-if-large {
        display: none;
    }
}
</style>
