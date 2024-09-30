<script setup lang="ts">
import { Ref, ref, watch } from 'vue';
import { useRoute, useRouter } from 'vue-router'

import { DateTime } from 'luxon';

import CalendarComponent from '../components/CalendarComponent.vue';

const route = useRoute();
const router = useRouter();

const currentYearMonthDayString: Ref<string> = ref(DateTime.now().toFormat('yyyyMMdd'));

function onDaySelected(yearMonthDayString: string) {
    router.push('/day/' + yearMonthDayString);
    currentYearMonthDayString.value = yearMonthDayString;
}

function onRouteChange(newRoute: typeof route) {
    if (newRoute.params.yyyymmdd) {
        const param = route.params.yyyymmdd as string;
        if (param !== currentYearMonthDayString.value) {
            currentYearMonthDayString.value = param;
        }
    }
}

watch(route, (newRoute: typeof route) => {
    onRouteChange(newRoute)
});

onRouteChange(route);

</script>

<template>
    <div>
        <CalendarComponent :year-month-day="currentYearMonthDayString" @day-selected="onDaySelected">
        </CalendarComponent>
    </div>
</template>

<style scoped></style>
