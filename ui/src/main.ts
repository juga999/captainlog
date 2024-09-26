import { createApp } from 'vue';

import { createWebHistory, createRouter, RouteRecordRaw } from 'vue-router';

import { DateTime } from 'luxon';

import { createI18n } from 'vue-i18n';

import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome';
import { library } from '@fortawesome/fontawesome-svg-core';
import { 
    faChevronLeft, faChevronRight,
    faLocationPin
} from '@fortawesome/free-solid-svg-icons';

import './style.css'
import App from './App.vue'

import DayView from './views/DayView.vue'
//import LoginView from './views/LoginView.vue'

library.add(faChevronLeft, faChevronRight, faLocationPin);

const routes: Array<RouteRecordRaw> = [
    { path: '/day/:yyyymmdd?', name: 'day', component: DayView },
    {
        path: '/', redirect: _ => {
            const today: string = DateTime.now().toFormat('yyyyMMdd');
            return '/day/' + today;
        }
    }
];

const router = createRouter({
    history: createWebHistory(),
    routes,
});

const i18n = createI18n({
    locale: 'fr'
})

createApp(App)
    .component('font-awesome-icon', FontAwesomeIcon)
    .use(router)
    .use(i18n)
    .mount('#app');
