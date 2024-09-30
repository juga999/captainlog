<script setup lang="ts">
const props = defineProps(['predicate']);

</script>

<template>
    <span class="cl-input-validity">
        <span v-if="props.predicate" class="cl-input-validity-ok">
            <font-awesome-icon icon="fa-solid fa-check" class="icon" />
        </span>
        <span v-if="!props.predicate">
            <font-awesome-icon icon="fa-solid fa-asterisk" class="icon" />
        </span>
    </span>
</template>

<style lang="css" scoped>
.cl-input-validity {
    justify-self: start;
    color: var(--primary-text-color);
}

.cl-input-validity-ok {
    color: var(--valid-text-color);
}
</style>
