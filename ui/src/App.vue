<script setup lang="ts">
import { Ref, ref, watch } from 'vue';

import { useRoute, useRouter } from 'vue-router'

import { useI18n } from 'vue-i18n';

import componentI18n from './i18n/AppI18n';

const route = useRoute();
const router = useRouter();

const { t } = useI18n(componentI18n);

type NavigationMenuEntry = {
    toRouteName: string,
    iconClass: string,
    label: string,
    isActive: boolean,
}

type NavigationMenuPanel = {
    entries: Array<NavigationMenuEntry>,
    routeToEntryMap: Map<string, NavigationMenuEntry>,
    isMaximized: boolean
    activeEntry: NavigationMenuEntry | null
}

function newNavigationMenuPanel(): NavigationMenuPanel {
    const panel: NavigationMenuPanel = {
        entries: [],
        routeToEntryMap: new Map<string, NavigationMenuEntry>(),
        isMaximized: false,
        activeEntry: null
    };

    const journalEntry: NavigationMenuEntry = {
        toRouteName: 'day',
        iconClass: 'fa-regular fa-clock',
        label: t('journal'),
        isActive: false
    }
    panel.entries.push(journalEntry);
    panel.routeToEntryMap.set('day', journalEntry);

    const projectsEntry: NavigationMenuEntry = {
        toRouteName: 'project.list',
        iconClass: 'fa-solid fa-diagram-project',
        label: t('projects'),
        isActive: false
    }
    panel.entries.push(projectsEntry);
    panel.routeToEntryMap.set('project', projectsEntry);

    return panel;
}

const navigationMenuPanel: Ref<NavigationMenuPanel> = ref(newNavigationMenuPanel());

function toggleNavigationPanel() {
    navigationMenuPanel.value.isMaximized = !navigationMenuPanel.value.isMaximized;
} 

function onNavigationEntryClick(menuEntry: NavigationMenuEntry) {
    router.push({name: menuEntry.toRouteName});
    navigationMenuPanel.value.isMaximized = false;
}

function onRouteChange(newRoute: typeof route) {
    const name = newRoute.name?.toString() || '';
    const rootName = name.split('.')[0];
    if (navigationMenuPanel.value.activeEntry) {
        navigationMenuPanel.value.activeEntry.isActive = false;
    }
    const entry = navigationMenuPanel.value.routeToEntryMap.get(rootName);
    if (entry) {
        entry.isActive = true;
        navigationMenuPanel.value.activeEntry = entry;
    }
}

watch(route, (newRoute: typeof route) => {
    onRouteChange(newRoute)
});

onRouteChange(route);

</script>

<template>
    <main class="app-wrapper">
        <nav class="app-navigation" :class="{
                'maximized-navigation': navigationMenuPanel.isMaximized,
                'minimized-navigation': !navigationMenuPanel.isMaximized
        }">
            <div class="app-navigation-main-icon">
            </div>
            <div class="app-navigation-entries">
                <div v-for="menuEntry in navigationMenuPanel.entries">
                    <div @click="onNavigationEntryClick(menuEntry)" :class="{
                        'app-navigation-entry-active': menuEntry.isActive
                    }" class="app-navigation-entry">
                        <div class="app-navigation-entry-icon"><font-awesome-icon :icon=menuEntry.iconClass
                                class="icon" /></div>
                        <div class="app-navigation-entry-text">{{ menuEntry.label }}</div>
                    </div>
                </div>
            </div>
            <div class="app-navigation-footer">
                <button id="app-navigation-toggle-btn" @click="toggleNavigationPanel()">
                    <span><font-awesome-icon icon="fa-solid fa-angles-right" class="icon" /></span>
                </button>
            </div>
        </nav>
        <div class="app-content">
            <RouterView></RouterView>
        </div>
    </main>
</template>

<style scoped>
.app-wrapper {
    --navigation-minimized-width: 4.5em;
    --navigation-maximized-width: 12em;
}

.app-navigation {
    position: fixed;
    top: 0;
    color: var(--primary-text-color);
    height: 100vh;
    transition: width .150s linear;
    background: linear-gradient(318deg, var(--dark-bg-color) 0%, var(--darker-bg-color) 100%);
}

.minimized-navigation {
    width: var(--navigation-minimized-width);
}

.maximized-navigation {
    width: var(--navigation-maximized-width);
}

.app-navigation-main-icon {
    height: 120px;
}

.app-navigation-footer {
    position: absolute;
    bottom: 0;
}

.app-navigation-entries {
    display: flex;
    flex-direction: column;
    row-gap: 2em;
}

.app-navigation-entry {
    height: 4em;
    display: flex;
    justify-content: space-between;
    align-items: center;
    transition: width .150s linear;
}

.app-navigation-entry:hover:not(.app-navigation-entry-active) {
    cursor: pointer;
    border-right: 8px solid var(--hover-color);
    background-color: var(--hover-color);
}

.app-navigation-entry-icon {
    padding-left: 0.3em;
    font-size: 2.2em;
}

.app-navigation-entry-text {
    width: 100%;
    padding-left: 1.2em;
    font-size: 1.3em;
    transition:visibility 0.150s linear,opacity 0.150s linear;
}

.maximized-navigation .app-navigation-entry-text {
    visibility: visible;
    opacity: 1;
}

.minimized-navigation .app-navigation-entry-text {
    visibility: hidden;
    opacity: 0;
}

.app-navigation-entry-active {
    border-right: 8px solid var(--secondary-aux-text-color);
    background-color: var(--less-less-dark-bg-color);
    color: var(--secondary-aux-text-color);
}

.minimized-navigation .app-navigation-entry {
    width: calc(var(--navigation-minimized-width) - 8px);
}

.maximized-navigation .app-navigation-entry {
    width: calc(var(--navigation-maximized-width) - 8px);
}

.app-navigation-footer {
    padding-bottom: 2em;
    width: 100%;
    text-align: center;
}

#app-navigation-toggle-btn {
    width: 100%;
    height: 2.6em;
    text-align: center;
    color: var(--primary-text-color);
    background-color: transparent;
    border: none;
    transition: background-color 0.150s linear;
}

.maximized-navigation #app-navigation-toggle-btn {
    transform: rotate(180deg);
}

#app-navigation-toggle-btn:hover {
    cursor: pointer;
    background-color: var(--hover-color);
}

#app-navigation-toggle-btn span {
    font-size: 2.2em;
}

.app-content {
    padding-top: 1em;
    padding-right: 1em;
    padding-left: calc(var(--navigation-minimized-width) + 1em);
    height: calc(100vh - 1em);
}

</style>