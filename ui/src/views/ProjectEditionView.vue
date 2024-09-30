<script setup lang="ts">
import { Reactive, reactive, computed } from 'vue';

import ProjectViewHeader from '../components/ProjectViewHeader.vue';
import RequiredValidityIcon from '../components/RequiredValidityIcon.vue';

type Project = {
    id: number | null,
    name: string,
    accountingCode: string,
    logoUrl: string | null
}

const project: Reactive<Project> = reactive({
    id: null,
    name: '',
    accountingCode: '',
    logoUrl: null
});

const isProjectNameValid = computed(() => {
    return project.name?.length > 1 || false;
});

const isProjectAccountingCodeValid = computed(() => {
    return project.accountingCode?.length > 1 || false;
});

let fileInput: HTMLInputElement | null = null;

function onProjectLogoChange(e: Event) {
    fileInput = e.target as HTMLInputElement;
    if (fileInput.files && fileInput.files.length === 1) {
        project.logoUrl = URL.createObjectURL(fileInput.files[0]);
    }
}

function onRemoveLogoClick() {
    if (fileInput) {
        fileInput.value = '';
    }
    project.logoUrl = null;
}

</script>

<template>
    <div class="cl-project-edition-container">
        <ProjectViewHeader>
            <template v-slot:header-title>
                <span class="cl-header-title-n1">
                    Projets
                    &nbsp;<font-awesome-icon icon="fa-solid fa-chevron-right" class="icon" />&nbsp;
                </span>
                <span class="cl-header-title-n2">Nouveau projet</span>
            </template>
        </ProjectViewHeader>

        <div class="cl-project-edition-body">
            <div class="cl-project-edition-form-wrapper">
                <form class="cl-project-edition-form">
                    <label>Nom</label>
                    <input name="projectName" type="text"
                        v-model="project.name"
                        required>
                    <RequiredValidityIcon :predicate=isProjectNameValid></RequiredValidityIcon>

                    <label>Code comptable</label>
                    <input name="projectAccountingCode" type="text"
                        v-model="project.accountingCode"
                        required>
                    <RequiredValidityIcon :predicate=isProjectAccountingCodeValid></RequiredValidityIcon>

                    <label>Logo</label>
                    <div id="project-logo-wrapper">
                        <div v-show="!project.logoUrl">
                            <input name="projectLogo" type="file" accept="image/*" @change="onProjectLogoChange">
                        </div>
                        <div v-if="project.logoUrl" id="project-logo-preview">
                            <img :src="project.logoUrl" />
                        </div>
                        <div v-if="project.logoUrl" id="project-logo-delete-btn">
                            <button type="button" @click="onRemoveLogoClick">
                                <font-awesome-icon icon="fa-solid fa-eraser" class="icon" />&nbsp;Retirer ce logo
                            </button>
                        </div>
                    </div>
                    <RequiredValidityIcon v-if="project.logoUrl" predicate="true"></RequiredValidityIcon>
                </form>
            </div>
        </div>

        <div class="cl-project-edition-footer">
            TODO
        </div>
    </div>
</template>

<style scoped>
.cl-project-edition-footer {
    padding-top: 0.2em;
    padding-bottom: 0.2em;
    padding-left: 1em;
    border-top: 1px solid var(--less-dark-bg-color);
    border-bottom-left-radius: 16px;
    border-bottom-right-radius: 16px;
    display: flex;
    flex-flow: row nowrap;
    justify-content: space-between;
    align-items: center;
}
/** Fin truc à refactoriser  */

.cl-project-edition-container {
    max-width: 1200px;
    border-radius: 16px;
    background-color: var(--less-less-dark-bg-color);
    box-shadow: rgba(0, 0, 0, 0.35) 0px 5px 15px;
}

.cl-project-edition-form-wrapper {
    padding-top: 1em;
    padding-bottom: 1em;
}

.cl-project-edition-form {
    display: grid;
    max-width: 60em;
}

.cl-project-edition-form label {
    color: var(--primary-text-color);
}

.cl-project-edition-form input[type=text] {
    border-top: none;
    border-left: none;
    border-right: none;
    border-bottom: 1px solid var(--primary-text-color);
    background: none;
    color: var(--primary-text-color);
    font-size: 1.2em;
}

.cl-project-edition-form input[type=text]:focus {
    outline: none;
    color: var(--secondary-text-color);
    border-bottom: 1px solid var(--secondary-text-color);
}

.cl-project-edition-form input[type=file] {
    color: var(--primary-text-color);
    font-size: 1em;
} 

.cl-project-edition-form input[type=file]::file-selector-button {
    border: 1px solid var(--primary-text-color);
    border-radius: 2em;
    padding-top: 0.5em;
    padding-bottom: 0.5em;
    padding-left: 1em;
    padding-right: 1em;
    background-color: var(--dark-bg-color);
    color: var(--primary-text-color);
    font-size: 1.1em;
}

.cl-project-edition-form input[type=file]::file-selector-button:hover {
    cursor: pointer;
    border: 1px solid var(--hover-color);
    background-color: var(--hover-color);
}

#project-logo-wrapper {
    justify-self: center;
}

#project-logo-preview {
    margin-top: 0.5em;
    margin-bottom: 0.5em;
    padding: 1em;
    background-color: var(--dark-bg-color);
    border: 1px solid var(--dark-bg-color);
    border-radius: 0.8em;
    text-align: center;
}

#project-logo-preview img{
    max-width: 200px;
}

#project-logo-delete-btn {
    text-align: center;
}

#project-logo-delete-btn button {
    border: 1px solid var(--primary-text-color);
    border-radius: 2em;
    padding-top: 0.5em;
    padding-bottom: 0.5em;
    padding-left: 1em;
    padding-right: 1em;
    background-color: var(--dark-bg-color);
    color: var(--primary-text-color);
    font-size: 0.9em;
}

#project-logo-delete-btn button:hover {
    cursor: pointer;
    border: 1px solid var(--hover-color);
    background-color: var(--hover-color);
}

@media screen and (min-width: 500px) {
    .cl-project-edition-form {
        grid-template-columns: 0.5fr 1fr 0.1fr;
        column-gap: 1em;
        row-gap: 2em;
    }

    .cl-project-edition-form label {
        justify-self: end;
    }

    .cl-project-edition-form input {
        max-width: 30em;
    }
}

@media screen and (max-width: 499px) {
    .cl-project-edition-form-wrapper {
        padding-left: 6px;
        padding-right: 6px;
    }

    .cl-project-edition-form {
        grid-template-columns: 1fr;
        row-gap: 1em;
    }

    .cl-project-edition-form label {
        justify-self: start;
    }
}

</style>
