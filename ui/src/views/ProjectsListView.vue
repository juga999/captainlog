<script setup lang="ts">

import { useRouter } from 'vue-router'

import ProjectHeader from '../components/ProjectViewHeader.vue';

const router = useRouter();

function onAddProjectClick() {
    router.push({name: 'project.new'});
}

</script>

<template>
    <div class="cl-projects-view-container">
        <ProjectHeader>
            <template v-slot:header-title>
                <span>Projets</span>
            </template>
            <template v-slot:header-actions>
                <button @click="onAddProjectClick">
                    <font-awesome-icon icon="fa-solid fa-plus" class="icon" />
                    Ajouter un projet
                </button>
            </template>
        </ProjectHeader>

        <div class="cl-projects-view-body">
            <div class="cl-projects-list">
                <div class="cl-projects-list-empty">
                    Il n'y a pas encore de projet
                </div>
            </div>
        </div>
    </div>
</template>

<style scoped>
.cl-projects-view-body {
    background-color: var(--darker-bg-color);
    border-left: 1px solid var(--dark-bg-color);
    border-right: 1px solid var(--dark-bg-color);
}

.cl-projects-list {
    /*width: 100%;*/
    background: linear-gradient(318deg, var(--less-less-dark-bg-color) 0%, var(--less-dark-bg-color) 100%);
    border: 1px solid var(--less-less-dark-bg-color);
}

.cl-projects-list-empty {
    text-align: center;
    color: var(--primary-text-color);
    font-size: 1.6em;
    padding-top: 2em;
    padding-bottom: 2em;
}
</style>
