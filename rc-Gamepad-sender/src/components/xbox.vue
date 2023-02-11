<template>
    <div class="control-Pad" :style="{ width: controlPadWidth + 'px', height: controlPadHeight + 'px' }">
        <div class="left-Thumb-Stick" :style="{ width: controlPadWidth * 0.3 + 'px', height: controlPadWidth * 0.3 + 'px'}">
            <div class="left-Thumb-Stick-point" :style="{ left: leftThumbStickX + '%', top: leftThumbStickY + '%' }"></div>
        </div>
        <div class="throttle-Pad" :style="{ width: controlPadWidth * 0.1 + 'px', height: controlPadWidth * 0.3 + 'px'}">
            <div class="throttle" :style="{ bottom: throttle + '%'}"></div>
        </div>
        <div class="button-Pad" :style="{ width: controlPadWidth * 0.2 + 'px', height: controlPadWidth * 0.2 + 'px', fontSize: controlPadWidth * 0.05 + 'px'}">
            <div class="button Y" :class="{ pressed: isPressY }">Y</div>
            <div class="button X" :class="{ pressed: isPressX }">X</div>
            <div class="button B" :class="{ pressed: isPressB }">B</div>
            <div class="button A" :class="{ pressed: isPressA }">A</div>
        </div>
        <div class="status-Bar" :style="{ width: controlPadWidth * 0.8 + 'px', height: '50px', fontSize: width / 50 + 'px'}">  
            <p class="info" >服务器连接状态:</p>
            <div class="status" :class="{ 'status-off': !gamepad.mqttConnected }"></div>
            <p class="info" >手柄连接状态:</p>
            <div class="status" :class="{ 'status-off': !gamepad.connect }"></div>
            <p class="info" >ch3:</p>
            <div class="status" :class="{ 'status-off': !gamepad.ch3 }"></div>
            <p class="info" >ch4:</p>
            <div class="status" :class="{ 'status-off': !gamepad.ch4 }"></div>
            <config :configData="configData" @config-data-Change="sendconfigData"></config>
        </div>
    </div>
</template>

<script>
import config from "./config.vue"
export default {
    name: 'Gamepad',
    components: {
        config
    },
    props : ["gamepad", "configData"],
    emits: ['configDataChange'],
    data() {
        return {
            width: 800,
            height: 500,
            max_width: 1000,
            max_height: 400,
            min_width: 500,
            min_height: 180,
            fixed: 4,
        }
    },
    computed: {
        controlPadWidth() {
            if(this.width > this.max_width) this.width = this.max_width;
            else if(this.width < this.min_width) this.width = this.min_width;
            return this.width;
        },
        controlPadHeight() {
            if(this.height > this.max_height) this.height = this.max_height;
            else if(this.height < this.min_height) this.height = this.min_height;
            return this.height;
        },
        leftThumbStickX() {
            var x = this.gamepad.axes[0].toFixed(this.fixed) * (35) + 35;
            return x;
        },
        leftThumbStickY() {
            var y = this.gamepad.axes[1].toFixed(this.fixed) * (35) + 35;
            return y;
        },
        throttle() {
            var rt;
            if(this.gamepad.throttle !== undefined) {
                rt = this.gamepad.throttle.toFixed(this.fixed) * 90;
            } else {
                rt = 45;
            }
            return rt;
        },
        isPressY() {
            var value;
            if(this.gamepad.buttons['Y'] !== undefined) {
                value = this.gamepad.buttons["Y"].value;
            } else {
                value = 0;
            }
            return value;
        },
        isPressX() {
            var value;
            if(this.gamepad.buttons['X'] !== undefined) {
                value = this.gamepad.buttons["X"].value;
            } else {
                value = 0;
            }
            return value;
        },
        isPressB() {
            var value;
            if(this.gamepad.buttons['B'] !== undefined) {
                value = this.gamepad.buttons["B"].value;
            } else {
                value = 0;
            }
            return value;
        },
        isPressA() {
            var value;
            if(this.gamepad.buttons['A'] !== undefined) {
                value = this.gamepad.buttons["A"].value;
            } else {
                value = 0;
            }
            return value;
        },
            
    },
    methods: {
        sendconfigData(data) {
            console.log(data)
            this.$emit('configDataChange', data)
        }
    },
    mounted() {
        window.onresize = () => {
            this.width = window.innerWidth;
            this.height =window.innerHeight;
        }
    },
    destroyed() {
		// 组件销毁后解绑事件
		window.onresize = null;
	}

}
</script>

<style scoped>
* {margin: 0; padding: 0;}
.control-Pad {
    position: relative;
    left: 0; top: 0;
    /* background-color: rgb(143, 143, 143); */
}
.left-Thumb-Stick {
    position: absolute;
    left: 5%; top: 40%;
    border-radius: 50%;
    background-color: rgb(198, 217, 236,0.7);
}
.left-Thumb-Stick-point {
    position: absolute;
    width: 30%; height:30%;
    background-color: rgb(148, 148, 148,0.7);
    border-radius: 50%;
}
.throttle-Pad {
    position: absolute;
    left: 80%; top: 40%;
    background-color: rgb(198, 217, 236,0.7);
}
.throttle {
    position: absolute;
    width: 100%; height: 10%;
    background-color: rgb(148, 148, 148,0.7);
}
.button-Pad {
    position: absolute;
    left: 50%; top: 40%;
}
.button {
    position: absolute;
    width: 30%; height:30%;
    background-color: rgb(198, 217, 236,0.7);
    display: flex;
    align-items: center;
    justify-content:center;
    border-radius: 50%;
    color:rgba(105, 144, 183, 0.7);
}
.pressed {
    background-color: rgba(160, 177, 193, 0.7);
}
.Y {
    left:35%; top: 2.5%;
}
.X {
    left:2.5%; top: 35%;
}
.B {
    left:67.5%; top: 35%;
}
.A {
    left:35%; top: 67.5%;
}
.status-Bar {
    position: absolute;
    display: flex;
    justify-content: center;
    align-items: center;
    left: 10%; top: 5%;
}
.status {
    width: 10px; height:10px;
    margin:20px 30px 20px 10px;
    background-color:rgba(51, 204, 51, 0.906);
    border-radius: 50%;
}
.status-off {
    background-color: rgb(240, 0, 12);
}
.info {
    line-height:50px;
    color: rgb(0, 0, 0,0.7);
}
</style>

