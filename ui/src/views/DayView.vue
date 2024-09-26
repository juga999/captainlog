<script setup lang="ts">
import { Ref, ref } from 'vue';
import { useRoute, useRouter } from 'vue-router'

import { DateTime } from 'luxon';

import CalendarComponent from '../components/CalendarComponent.vue';

const route = useRoute();
const router = useRouter();

const currentYearMonthDayString: Ref<string> = ref(DateTime.now().toFormat('yyyyMMdd'));
if (route.params.yyyymmdd) {
    currentYearMonthDayString.value = route.params.yyyymmdd as string;
}

function onDaySelected(yearMonthDayString: string) {
    router.push('/day/' + yearMonthDayString);
    currentYearMonthDayString.value = yearMonthDayString;
}

</script>

<template>
    <div>
        <CalendarComponent :year-month-day="currentYearMonthDayString" @day-selected="onDaySelected">
        </CalendarComponent>
    </div>
</template>

<style scoped></style>
