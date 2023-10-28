<script setup>
import {
    // DashboardSectionsSummary,
    DashboardBasesSummary,

    // DashboardRecents,
    // DashboardRanking,
    // DashboardOverviewChart,
    DashboardNotification
} from '@/components/dashboard';

// import { onMounted, reactive, ref, watch } from 'vue';
// import ProductService from '@/service/ProductService';
// import { useLayout } from '@/layout/composables/layout';

// const { isDarkTheme } = useLayout();

// const products = ref(null);

// const productService = new ProductService();

// onMounted(() => {
//     productService.getProductsSmall().then((data) => (products.value = data));
// });

// const formatCurrency = (value) => {
//     return value.toLocaleString('en-US', { style: 'currency', currency: 'USD' });
// };
// const applyLightTheme = () => {
//     lineOptions.value = {
//         plugins: {
//             legend: {
//                 labels: {
//                     color: '#495057'
//                 }
//             }
//         },
//         scales: {
//             x: {
//                 ticks: {
//                     color: '#495057'
//                 },
//                 grid: {
//                     color: '#ebedef'
//                 }
//             },
//             y: {
//                 ticks: {
//                     color: '#495057'
//                 },
//                 grid: {
//                     color: '#ebedef'
//                 }
//             }
//         }
//     };
// };

// const applyDarkTheme = () => {
//     lineOptions.value = {
//         plugins: {
//             legend: {
//                 labels: {
//                     color: '#ebedef'
//                 }
//             }
//         },
//         scales: {
//             x: {
//                 ticks: {
//                     color: '#ebedef'
//                 },
//                 grid: {
//                     color: 'rgba(160, 167, 181, .3)'
//                 }
//             },
//             y: {
//                 ticks: {
//                     color: '#ebedef'
//                 },
//                 grid: {
//                     color: 'rgba(160, 167, 181, .3)'
//                 }
//             }
//         }
//     };
// };

// watch(
//     isDarkTheme,
//     (val) => {
//         if (val) {
//             applyDarkTheme();
//         } else {
//             applyLightTheme();
//         }
//     },
//     { immediate: true }
// );
</script>

<template>
    <div class="grid">
        <DashboardBasesSummary />

        <div class="col-12 xl:col-6">
            <!-- <div class="card">
                <DashboardRecents />
            </div> -->
            <!-- <div class="card">
                <DashboardRanking />
            </div> -->
        </div>
        <div class="col-12 xl:col-6">
            
            <!-- <DashboardOverviewChart :data="lineData" :options="lineOptions" /> -->
            <!-- <DashboardOverviewChart /> -->

            <DashboardNotification />
        </div>
    </div>
</template>
