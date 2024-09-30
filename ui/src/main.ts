import { createApp } from 'vue';

import { createWebHistory, createRouter, RouteRecordRaw } from 'vue-router';

import { DateTime } from 'luxon';

import { createI18n } from 'vue-i18n';

import { FontAwesomeIcon } from '@fortawesome/vue-fontawesome';
import { library } from '@fortawesome/fontawesome-svg-core';
import { 
    faClock
} from '@fortawesome/free-regular-svg-icons';
import {
    faChevronLeft, faChevronRight, faAnglesRight, faPlus,
    faAsterisk, faCheck, faEraser,
    faLocationPin, faDiagramProject
} from '@fortawesome/free-solid-svg-icons';

import './style.css';
import App from './App.vue';

import DayView from './views/DayView.vue';

import ProjectsView from './views/ProjectsView.vue';
import ProjectsListComponent from './views/ProjectsListView.vue';
import ProjectEditionComponent from './views/ProjectEditionView.vue';

library.add(
    faClock, faChevronLeft, faChevronRight, faAnglesRight,
    faPlus, faLocationPin, faDiagramProject, faAsterisk, faCheck, faEraser);

const routes: Array<RouteRecordRaw> = [
    { 
        path: '/day/:yyyymmdd?', name: 'day', component: DayView
    },
    { 
        path: '/project', component: ProjectsView,
        children: [
            { path: '', name: 'project.list', component: ProjectsListComponent },
            { path: 'new', name: 'project.new', component: ProjectEditionComponent },
            { path: ':projectId', name: 'project.details', component: ProjectsView }
        ]
    },
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
