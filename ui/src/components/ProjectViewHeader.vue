<script setup lang="ts">
</script>

<template>
    <div class="cl-projects-view-header">
        <div class="cl-projects-view-header-title">
            <slot name="header-title"></slot>
        </div>
        <div class="cl-projects-view-header-actions">
            <slot name="header-actions"></slot>
        </div>
    </div>
</template>

<style>
.cl-projects-view-header {
    padding-top: 0.2em;
    padding-bottom: 0.2em;
    padding-left: 1em;
    border-top-left-radius: 16px;
    border-top-right-radius: 16px;
    display: flex;
    flex-flow: row nowrap;
    justify-content: space-between;
    align-items: center;
}

.cl-projects-view-header-title {
    color: var(--secondary-aux-text-color);
    font-size: 2em;
}

.cl-projects-view-header-actions {
    padding-right: 1em;
}

.cl-projects-view-header-actions button {
    border: 1px solid var(--less-less-dark-bg-color);
    border-radius: 2em;
    padding-top: 0.5em;
    padding-bottom: 0.5em;
    padding-left: 1em;
    padding-right: 1em;
    background-color: var(--dark-bg-color);
    color: var(--primary-text-color);
    font-size: 1.1em;
}

.cl-projects-view-header-actions button:hover {
    cursor: pointer;
    background-color: var(--hover-color);
    border-color: var(--hover-color);
}

.cl-header-title-n1 {
    color: var(--secondary-aux-text-color);
}

.cl-header-title-n2 {
    color: var(--secondary-text-color);
}

</style>
