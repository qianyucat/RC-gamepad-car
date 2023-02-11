<template>
    <n-button @click="onButtonClick" >
    设置
    </n-button>
    <n-modal v-model:show="showModal" preset="dialog">
            <template #header>
                参数调整
            </template>
            <n-form
                ref="formRef"
                :model="data"
                label-placement="left"
                label-width="auto"
                require-mark-placement="right-hanging"
                size= "medium"
                :rules="rules"
                :style="{
                    maxWidth: '640px'
                }"
            >
                <n-form-item label="MQTT服务器地址：" path="server">
                    <n-input v-model:value="data.server" placeholder="IP地址或域名" />
                </n-form-item>
                <n-form-item label="MQTT端口号：" path="port">
                    <n-input v-model:value="data.port" placeholder="1883" />
                </n-form-item>
                <n-form-item label="MQTT用户名：" path="user">
                    <n-input v-model:value="data.user" placeholder="服务器用户名（没有可不写）" />
                </n-form-item>
                <n-form-item label="MQTT密码：" path="password">
                    <n-input v-model:value="data.password" placeholder="服务器密码（没有可不写）" />
                </n-form-item>
                <n-form-item label="RC遥控车名：" path="carName">
                    <n-input v-model:value="data.carName" placeholder="取一个与众不同的名字" />
                </n-form-item>
            </n-form>
            最大方向：
            <n-slider v-model:value="data.dMax" :min=1500 :max=2500 :step="1" />
            最大油门：
            <n-slider v-model:value="data.eMax" :min=1500 :max=2500 :step="1" />
            方向中位点微调：
            <n-slider v-model:value="data.dTrim" :min=-100 :max=100 :step="1" />
            油门中位点微调：
            <n-slider v-model:value="data.eTrim" :min=-100 :max=100 :step="1" />
            <template #action>
                    <n-button strong secondary type="info" @click="closeTrim">取消</n-button>
                    <n-button strong secondary type="success" @click="$emit('configDataChange', data)">确认</n-button>
            </template>
    </n-modal>
</template>

<script>
    import { NButton, NModal, NSlider, NForm, NFormItem, NInput } from 'naive-ui'
    export default {
        components: {
            NButton, NModal, NSlider, NForm, NFormItem, NInput
        },
        props : ['configData'],
        emits: ['configDataChange'],
        data() {
            return {
                showModal: false,
                data: {
                    server: this.configData.server,
                    port: this.configData.port,
                    user: this.configData.user,
                    password: this.configData.password,
                    carName: this.configData.carName,
                    dMax: this.configData.dMax,
                    eMax: this.configData.eMax,
                    dTrim: this.configData.dTrim,
                    eTrim: this.configData.eTrim
                },
                rules: {
                        server: {
                            required: true,
                            message: '请输入MQTT服务器地址',
                            trigger: 'blur'
                        },
                        port: {
                            required: true,
                            message: '请输入MQTT端口号',
                            trigger: 'blur'
                        },
                        user: {
                            required: false,
                            message: '',
                            trigger: ''
                        },
                        password: {
                            required: false,
                            message: '',
                            trigger: ''
                        },
                        carName: {
                            required: true,
                            message: '请输入RC遥控车名',
                            trigger: 'blur'
                        }
                    
                }
            }
        },
        watch: {

        },
        computed: {

        },
        methods: {
            onButtonClick() {
                this.showModal = true;
                this.data.server = this.configData.server,
                this.data.port = this.configData.port,
                this.data.user = this.configData.user,
                this.data.password = this.configData.password,
                this.data.carName = this.configData.carName,
                this.data.dMax = this.configData.dMax;
                this.data.eMax = this.configData.eMax;
                this.data.dTrim = this.configData.dTrim;
                this.data.eTrim = this.configData.eTrim;
            },
            closeTrim() {
                this.showModal = false;
                this.data.dMax = this.configData.dMax;
                this.data.eMax = this.configData.eMax;
                this.data.dTrim = this.configData.dTrim;
                this.data.eTrim = this.configData.eTrim;
            }
        },
        mounted() {

        }
    }
</script>

<style scoped>

</style>