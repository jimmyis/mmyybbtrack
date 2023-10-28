<script setup>
import { ref } from 'vue';

const products = ref(null);
</script>

<template>
    <h5>Recent Sales</h5>
    <DataTable :value="products" :rows="5" :paginator="true" responsiveLayout="scroll">
        <Column style="width: 15%">
            <template #header> Image </template>
            <template #body="slotProps">
                <img :src="'demo/images/product/' + slotProps.data.image" :alt="slotProps.data.image" width="50" class="shadow-2" />
            </template>
        </Column>
        <Column field="name" header="Name" :sortable="true" style="width: 35%"></Column>
        <Column field="price" header="Price" :sortable="true" style="width: 35%">
            <template #body="slotProps">
                {{ formatCurrency(slotProps.data.price) }}
            </template>
        </Column>
        <Column style="width: 15%">
            <template #header> View </template>
            <template #body>
                <Button icon="pi pi-search" type="button" class="p-button-text"></Button>
            </template>
        </Column>
    </DataTable>
</template>