<template>
    <xbox :gamepad="gamepad"></xbox>
  <ul>
    <li v-for="(value, key) in gamepad.axes">
      {{ key }}: {{ value }}
    </li>
  </ul>
  <ul>
    <li v-for="(value, key) in gamepad.buttons">
      {{ key }}: {{ "pressed：" + value.pressed + " touched：" + value.touched + " value：" + value.value
      }}
    </li>
    {{ gamepad.throttle }}
  </ul>

</template>
<script>
import xbox from "./xbox.vue"
export default {
  name: 'Gamepad',
  components: {
    xbox
  },
  data() {
    return {
      interval: null,
      gp: null,
      gamepad: {
        connect: false,
        axes: [0,0,0,0],
        buttons: {},
        layout: [
          'A', 'B', 'X', 'Y', 'LB', 'RB', 'LT', 'RT',
          'Back', 'Start', 'Left-Click', 'Right-Click',
          'DPad-Up', 'DPad-Down', 'DPad-Left', 'DPad-Right'
        ],
        throttle: 0.5,
        ch3: false,
        ch4: false,
        mqttConnected: false
      },
      rcCarID: "rc-gamepad-car",
      connection: {
        protocol: "ws",
        host: "192.168.31.12",
        // ws: 8083; wss: 8084
        port: 8083,
        endpoint: "/mqtt",
        // for more options, please refer to https://github.com/mqttjs/MQTT.js#mqttclientstreambuilder-options
        clean: true,
        connectTimeout: 30 * 1000, // ms
        reconnectPeriod: 4000, // ms
        clientId: "emqx_vue_" + Math.random().toString(16).substring(2, 8),
        // auth
        // username: "emqx_test",
        // password: "emqx_test",
      },
      subscription: {
        topic: "topic/mqttx",
        qos: 0,
      },
      publish: {
        topic: "topic/browser",
        qos: 0,
        payload: '{ "A": 0, "B": 1, "C": 0, "D": 0 }',
      },
      receiveNews: "",
      qosList: [0, 1, 2],
      client: {
        connected: false,
      },
      subscribeSuccess: false,
      connecting: false,
      retryTimes: 0,
      publishOnTimes: 0
    }
  },
  watch: {
    //监听按键是否按下
    'gamepad.buttons.Y': {
      handler(newButton, oldButton) {
        if(oldButton) {
          if(newButton.value == 1 && oldButton.value == 0) {
            this.gamepad.ch3 = !this.gamepad.ch3;
            console.log(this.gamepad.ch3);

            let payload =  "" + (this.gamepad.ch3 == true? 1 : 0);
            let qos = 1;
            // console.log(this.publish.payload);
            let topic = "rcCar/" + this.rcCarID + "/control/ch3";
            this.doPublish(topic, qos, payload);
          }
        }
      },
    },
    'gamepad.buttons.X': {
      handler(newButton, oldButton) {
        if(oldButton) {
          if(newButton.value == 1 && oldButton.value == 0) {
            this.gamepad.ch4 = !this.gamepad.ch4;
            console.log(this.gamepad.ch4);

            let payload =  "" + (this.gamepad.ch4 == true? 1 : 0);
            let qos = 1;
            // console.log(this.publish.payload);
            let topic = "rcCar/" + this.rcCarID + "/control/ch4";
            this.doPublish(topic, qos, payload);
          }
        }
      },
    },
    'gamepad.buttons.B': {
      handler(newButton, oldButton) {
        if(oldButton) {
          if(newButton.value == 1 && oldButton.value == 0) {
            
          }
        }
      },
    },
    'gamepad.buttons.A': {
      handler(newButton, oldButton) {
        if(oldButton) {
          if(newButton.value == 1 && oldButton.value == 0) {
            
          }
        }
      },
    }
  },
  methods: {
    connecthandler(e) {
      console.log(e.gamepad);
      console.log('控制器已连接');
      this.startGamepad(e.gamepad.index);
    },
    disconnecthandler(e) {
      console.log(e.gamepad);
      clearInterval(this.interval);
      console.log('控制器已断开');
      this.gamepad.connect = false;

      this.publish.payload = '{"A":0,"B":1,"C":0,"D":0}';
      this.publish.topic = "rcCar/" + this.rcCarID + "/control/motor";
      this.doPublish();
    },
    //开启定时器获取按键信息
    startGamepad(index) {
      this.interval = setInterval(() => {
        this.gp = navigator.getGamepads()[index];
        this.gamepad.connect = this.gp.connected;
        this.gamepad.axes = this.gp.axes;
        //添加buttons值到指定按键
        for(let i = 0; i < this.gp.buttons.length; i++) {
          this.gamepad.buttons[this.gamepad.layout[i]] = this.gp.buttons[i];
        }
        //LT刹车优先级高
        if (this.gamepad.buttons["LT"].touched == true) {
          this.gamepad.throttle = -this.gamepad.buttons["LT"].value / 2 + 0.5;
        } else {
          this.gamepad.throttle = this.gamepad.buttons["RT"].value / 2 + 0.5;
        }

        //发送信息
        // this.publish.payload = "" + (this.gamepad.connect == true? 1:0) + "," +
        //                         (this.gamepad.axes[0].toFixed(2) * 100 + 100) + "," + 
        //                         (this.gamepad.buttons["RT"].value.toFixed(2) * 100) + "," + 
        //                         (this.gamepad.forward == true? 1:0) + ",";
        // let buffer = new ArrayBuffer(4);
        // let view = new Int8Array(buffer);

        // view[0] = this.gamepad.connect == true? 1:0;
        // view[1] = this.gamepad.forward == true? 1:0;
        // view[2] = this.gamepad.axes[0].toFixed(2) * 100;
        // view[3] = this.gamepad.buttons["RT"].value.toFixed(2) * 100;
        // console.log(view);
        // var s = String.fromCharCode.apply(null, view);
        
        // console.log(s.charCodeAt(0).toString(16) + '/' +s.charCodeAt(1).toString(16) + '/' +s.charCodeAt(2).toString(16) + '/' +s.charCodeAt(3).toString(16));
        // this.publish.payload = buffer;
        if (this.publishOnTimes > 3) {
          this.publish.payload = JSON.stringify({
            "0": this.gamepad.connect == true? 1 : 0,
            "1": this.gamepad.axes[0].toFixed(3) * 500 + 500,
            "2": this.gamepad.throttle.toFixed(3) * 1000,
          })
          // console.log(this.publish.payload);
          this.publish.topic = "rcCar/" + this.rcCarID + "/control/motor";
          this.publish.qos = 0;
          this.doPublish(this.publish.topic, this.publish.qos, this.publish.payload);
          this.publishOnTimes = 0;
        } else {
          this.publishOnTimes++;
        }
      }, 16);
    },
    initData() {
      this.client = {
        connected: false,
      };
      this.retryTimes = 0;
      this.connecting = false;
      this.subscribeSuccess = false;
      this.gamepad.mqttConnected = false;
    },
    handleOnReConnect() {
      this.retryTimes += 1;
      if (this.retryTimes > 5) {
        try {
          this.client.end();
          this.initData();
          this.$message.error("Connection maxReconnectTimes limit, stop retry");
        } catch (error) {
          this.$message.error(error.toString());
        }
      }
    },
    createConnection() {
      try {
        this.connecting = true;
        const { protocol, host, port, endpoint, ...options } = this.connection;
        const connectUrl = `${protocol}://${host}:${port}${endpoint}`;
        this.client = mqtt.connect(connectUrl, options);
        if (this.client.on) {
          this.client.on("connect", () => {
            this.connecting = false;
            this.gamepad.mqttConnected = true;
            console.log("Connection succeeded!");
          });
          this.client.on("reconnect", this.handleOnReConnect);
          this.client.on("error", (error) => {
            console.log("Connection failed", error);
          });
          this.client.on("message", (topic, message) => {
            this.receiveNews = this.receiveNews.concat(message);
            console.log(`Received message ${message} from topic ${topic}`);
          });
        }
      } catch (error) {
        this.connecting = false;
        console.log("mqtt.connect error", error);
      }
    },
    doSubscribe() {
      const { topic, qos } = this.subscription
      this.client.subscribe(topic, { qos }, (error, res) => {
        if (error) {
          console.log('Subscribe to topics error', error)
          return
        }
        this.subscribeSuccess = true
        console.log('Subscribe to topics res', res)
      })
    },
    doPublish(topic, qos, payload) {
      this.client.publish(topic, payload, { qos }, error => {
        if (error) {
          console.log('Publish error', error)
        }
      })
    },
    destroyConnection() {
      if (this.client.connected) {
        try {
          this.client.end(false, () => {
            this.initData()
            console.log('Successfully disconnected!')
          })
        } catch (error) {
          console.log('Disconnect failed', error.toString())
        }
      }
    },
    IntToBytes(number, length){
      var bytes = [];
      var i = length;
      do {
      bytes[--i] = number & 0xFF;
      number = number>>8;
      } while (i)
      return bytes;
    }
  },
  mounted() {
    window.addEventListener("gamepadconnected", this.connecthandler);
    window.addEventListener("gamepaddisconnected", this.disconnecthandler);
    this.createConnection();
  }
}
</script>