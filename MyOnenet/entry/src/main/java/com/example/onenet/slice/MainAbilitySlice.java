package com.example.onenet.slice;

import com.example.onenet.ResourceTable;
import ohos.aafwk.ability.AbilitySlice;
import ohos.aafwk.content.Intent;
import ohos.agp.components.*;
import ohos.agp.window.service.WindowManager;
import ohos.app.Context;
import ohos.eventhandler.EventHandler;
import ohos.eventhandler.EventRunner;
import ohos.eventhandler.InnerEvent;
import ohos.global.resource.Resource;
import ohos.hiviewdfx.HiLog;
import ohos.hiviewdfx.HiLogLabel;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Timer;
import java.util.TimerTask;


import com.google.gson.*;
import ohos.net.NetHandle;
import ohos.net.NetManager;
import ohos.net.NetStatusCallback;

import ohos.eventhandler.EventHandler;
import ohos.eventhandler.EventRunner;
import ohos.eventhandler.InnerEvent;


public class MainAbilitySlice extends AbilitySlice {
    private static final HiLogLabel LABEL = new HiLogLabel(HiLog.LOG_APP, 0x00201, "Landscape");
    private MyEventHandler myEventHandler;
    private Text textTem;
    private Text led_status;
    private Button buttonOn;
    private Button buttonOff;

    // 与onenet设备相关的配置
    private String api_key = "";
    String deviceID = "";
    String getUrl = "http://api.heclouds.com/devices/" + deviceID + "/datastreams/temperature";
    String postUrl = "http://api.heclouds.com/cmds?device_id=" + deviceID;

    // 定义一个Timer，用于重复获取Onenet数据并更新UI
    private Timer timer = new Timer();

    @Override
    public void onStart(Intent intent) {

        super.onStart(intent);
//        加载XML作为UI布局
        super.setUIContent(ResourceTable.Layout_ability_main);
        // 隐藏状态栏、设置状态栏和导航栏透明
        getWindow().addFlags(WindowManager.LayoutConfig.MARK_TRANSLUCENT_STATUS);

        buttonOn = (Button) findComponentById(ResourceTable.Id_button_on);
        buttonOff = (Button) findComponentById(ResourceTable.Id_button_off);
        led_status = (Text) findComponentById(ResourceTable.Id_led_status);

        //初始化线程类
        initHandler();

        textTem = (Text)findComponentById(ResourceTable.Id_text_tem);

        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                doGet(getUrl);
            }
        },0,500);// 每500ms执行一次任务，并更新UI

        buttonOn.setClickedListener(new Component.ClickedListener() {
            @Override
            public void onClick(Component component) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        doPost(postUrl, "102", 1102);
                    }
                }).start();
            }
        });

        buttonOff.setClickedListener(new Component.ClickedListener() {
            @Override
            public void onClick(Component component) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        doPost(postUrl, "100", 1100);
                    }
                }).start();
            }
        });

    }


    // 定义MyEventHandler类
    private void initHandler() {
        myEventHandler = new MyEventHandler(EventRunner.current());
    }

    private class MyEventHandler extends EventHandler {
        public MyEventHandler(EventRunner runner) throws IllegalArgumentException {
            super(runner);
        }

        @Override
        protected void processEvent(InnerEvent event) {
            super.processEvent(event);
            if(event==null){
                return;
            }
            //更新UI
            if(event.eventId==1002){
                JsonBean jsonBean = (JsonBean) event.object;
                float tem = jsonBean.getData().getCurrent_value();
                String temText = Float.toString(tem);
                textTem.setText(temText);
            }
            if(event.eventId==1102){
                led_status.setText("当前状态：亮");
            }
            if(event.eventId==1100){
                led_status.setText("当前状态：灭");
            }
        }

    }

//    解析Json的类
    public static class JsonBean{
        public int errno;
        public Data data;
        public String error;

        static class Data{
            public String update_at;
            public String id;
            public String create_time;
            public float current_value;

            public String getCreate_time() {
                return create_time;
            }

            public String getUpdate_at() {
                return update_at;
            }

            public String getId() {
                return id;
            }

            public float getCurrent_value() {
                return current_value;
            }
        }

        public int getErrno() {
            return errno;
        }

        public Data getData() {
            return data;
        }

        public String getError() {
            return error;
        }
    }

    public void doGet(String url) {
        NetManager manager = NetManager.getInstance(this);
        if(!manager.hasDefaultNet()){
            return;
        }
        //实现网络操作功能
        NetHandle netHandler = manager.getDefaultNet();
        manager.addDefaultNetStatusCallback(new NetStatusCallback(){
            //网络正常
            @Override
            public void onAvailable(NetHandle handle) {
                super.onAvailable(handle);
                HiLog.info(LABEL,"网络状况正常");

            }
            //网络阻塞
            @Override
            public void onBlockedStatusChanged(NetHandle handle, boolean blocked) {
                super.onBlockedStatusChanged(handle, blocked);
                HiLog.info(LABEL,"网络状况阻塞");
            }
        });

        HttpURLConnection httpURLConnection = null;
        InputStream in = null;
        BufferedReader br = null;
        String result = null;// 返回结果字符串

        try {
            // 创建远程url连接对象
            URL url1 = new URL(url);
            // 通过远程url连接对象打开一个连接，强转成httpURLConnection类
            httpURLConnection = (HttpURLConnection) url1.openConnection();
            // 设置连接方式：get
            httpURLConnection.setRequestMethod("GET");
            // 设置连接主机服务器的超时时间：15000毫秒
            httpURLConnection.setConnectTimeout(15000);
            // 设置读取远程返回的数据时间：60000毫秒
            httpURLConnection.setReadTimeout(60000);
            //设置格式
//            httpURLConnection.setRequestProperty("Content-type", "application/json");
            // 设置鉴权信息：Authorization: Bearer da3efcbf-0845-4fe3-8aba-ee040be542c0 OneNet平台使用的是Authorization+token
            httpURLConnection.setRequestProperty("api-key", api_key);
            // 发送请求
            httpURLConnection.connect();
            // 通过connection连接，获取输入流
            if (httpURLConnection.getResponseCode() == 200) {
                in = httpURLConnection.getInputStream();
                // 封装输入流is，并指定字符集
                br = new BufferedReader(new InputStreamReader(in, "UTF-8"));
                // 存放数据
                StringBuffer sbf = new StringBuffer();
                String temp = null;
                while ((temp = br.readLine()) != null) {
                    sbf.append(temp);
                    sbf.append("\r\n");
                }
                result = sbf.toString();
                System.out.println("response=" + result);

                try{
                    // 处理json数据
                    // 利用上面定义的JsonBean类，取得对应的key-value
                    JsonBean jsonBean = new Gson().fromJson(result, JsonBean.class);
                    //线程投递
                    myEventHandler.sendEvent(InnerEvent.get(1002, jsonBean));

                }catch(JsonIOException e){
                    e.printStackTrace();
                }catch (NullPointerException e){
                    e.printStackTrace();
                }



            }
        } catch (UnsupportedEncodingException unsupportedEncodingException) {
            unsupportedEncodingException.printStackTrace();
        } catch (ProtocolException protocolException) {
            protocolException.printStackTrace();
        } catch (MalformedURLException malformedURLException) {
            malformedURLException.printStackTrace();
        } catch (IOException ioException) {
            ioException.printStackTrace();
        } finally {
            // 关闭资源
            if (null != br) {
                try {
                    br.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (null != in) {
                try {
                    in.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            httpURLConnection.disconnect();// 关闭远程连接
        }
        return;
    }

    public void doPost(String url, String data, int reqCode){
        HttpURLConnection postConnection = null;
        URL postUrl;
        try{
            postUrl = new URL(url);
            postConnection = (HttpURLConnection) postUrl.openConnection();
            postConnection.setConnectTimeout(40000);
            postConnection.setReadTimeout(30000);
            postConnection.setRequestMethod("POST");
            //发送POST请求必须设置为true
            postConnection.setDoOutput(true);
            postConnection.setDoInput(true);
            // 设置二进制格式的数据
            postConnection.setRequestProperty("Content-Type", "application/octet-stream");
            postConnection.setRequestProperty("api-key", api_key);

            DataOutputStream dos = new DataOutputStream(postConnection.getOutputStream());
            dos.write(data.getBytes(StandardCharsets.UTF_8));
            // flush输出流的缓冲
            dos.flush();
            // 定义BufferReader输入流来读取URL的响应
            BufferedReader in = new BufferedReader(new InputStreamReader(postConnection.getInputStream()));
            String line;
            String result = "";
            while((line = in.readLine())!=null){
                result += line;
            }
            //线程投递
            myEventHandler.sendEvent(InnerEvent.get(reqCode));

        }catch (Exception e){
            e.printStackTrace();
        }


    }


    @Override
    public void onActive() {
        super.onActive();
    }

    @Override
    public void onForeground(Intent intent) {
        super.onForeground(intent);
    }
}
