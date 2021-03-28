package com.gc.app;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static com.gc.app.MyNativeRender.SAMPLE_TYPE;

public class MyGLSurfaceView extends GLSurfaceView implements ScaleGestureDetector.OnScaleGestureListener {
    private static final String TAG = "MyGLSurfaceView";

    private final float TOUCH_SCALE_FACTOR = 180.0f / 320;
    private final float TOUCH_DISPLACEMENT_FACTOR=1.0f / 320;

    public static final int IMAGE_FORMAT_RGBA = 0x01;
    public static final int IMAGE_FORMAT_NV21 = 0x02;
    public static final int IMAGE_FORMAT_NV12 = 0x03;
    public static final int IMAGE_FORMAT_I420 = 0x04;


    private float mPreviousY_Angle;
    private float mPreviousX_Angle;
    private int mXAngle;
    private int mYAngle;
    private float mXOffset=0.0f;
    private float mYOffset=0.0f;
    private float mPreviousX_Offset;
    private float mPreviousY_Offset;
    private MyGLRender mGLRender;

    private int mRatioWidth = 0;
    private int mRatioHeight = 0;

    private ScaleGestureDetector mScaleGestureDetector;
    private float mPreScale = 0.0f;
    private float mCurScale = 0.0f;
    private long mLastMultiTouchTime;

    private static int timeout=200;//双击间隔
    private int clickCount = 0;//记录连续点击次数
    private Handler handler;

    //左为false右为true
    public boolean display_deformation=false;
    public boolean rotate_translation=false;
    public boolean ffd_dffd=false;
    public boolean setLine_deformation=false;//绘制旋转面曲线还是变形阶段
    public int showBoundbox=0;
    public int wireFrame_fill=1;

    //旋转线相关数据
    public int screenWidth;
    public int screenHeight;
    private final float rotateLines_beginPosLimit=30.1f;
    private final float rotateLines_endPosLimit=30.1f;
    private final float rotateLines_spaceLimit_min=400.0f;//每个点的间距阈值（平方）
    private final float rotateLines_spaceLimit_max=40000.0f;//每个点的间距阈值（平方）
    private final int rotateLines_MinPointNums=15;

    private boolean rotateLines_hasBegin=false;
    public boolean rotateLines_hasEnd=false;
    private boolean rotateLines_Atright=true;//旋转线是否固定在Y轴的右侧
    private float rotateLines_previousX,rotateLines_previousY;
    private int rotateLines_PointNums=0;

    public MyGLSurfaceView(Context context) {
        this(context, null);
    }

    public MyGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.setEGLContextClientVersion(2);
        mGLRender = new MyGLRender();
        /*If no setEGLConfigChooser method is called,
        then by default the view will choose an RGB_888 surface with a depth buffer depth of at least 16 bits.*/
        setEGLConfigChooser(8, 8, 8, 8, 16, 8);
        setRenderer(mGLRender);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
        //setRenderMode(RENDERMODE_CONTINUOUSLY);
        mScaleGestureDetector = new ScaleGestureDetector(context, this);
        handler = new Handler();
    }

    public void reSetRotateLine(){
        rotateLines_hasBegin=false;
        rotateLines_hasEnd=false;
        rotateLines_Atright=true;//旋转线是否固定在Y轴的右侧
        rotateLines_previousX=0;
        rotateLines_previousY=0;
        rotateLines_PointNums=0;
    }
    //单点触控手势
    @Override
    public boolean onTouchEvent(final MotionEvent e) {
        //动作的x,y坐标是相对glsurfaceview区域的左上角，这个值可能为负（即在view的外部）
        //旋转线阶段的UpdateTransformMatrix后两个参数分别代表是否移动/是否结束，是为1否为0
        if(!setLine_deformation){
            if(rotateLines_hasEnd)
               return true;
            float y = screenHeight/2-e.getY();
            float x = e.getX()-screenWidth/2;
System.out.println(x+" , "+y);
            if(!rotateLines_hasBegin){
                if(Math.abs(x)<rotateLines_beginPosLimit&&x>0==rotateLines_Atright){
                    if(x==0)
                        x=0.1f;
                    mGLRender.UpdateTransformMatrix(0, y*2/screenHeight, 0, 0);
                    mGLRender.UpdateTransformMatrix(x*2/screenWidth, y*2/screenHeight, 0, 0);
                    rotateLines_PointNums+=2;

                    rotateLines_previousX=x;
                    rotateLines_previousY=y;
                    rotateLines_hasBegin=true;
                    requestRender();
                }
            }
            else{
                float distance=(float)Math.pow(y-rotateLines_previousY,2)+(float)Math.pow(x-rotateLines_previousX,2);
                if(distance>rotateLines_spaceLimit_min&&distance<rotateLines_spaceLimit_max&&(x>0==rotateLines_Atright)){
                    mGLRender.UpdateTransformMatrix(x*2/screenWidth, y*2/screenHeight, 1, 0);
                    rotateLines_PointNums++;
                    rotateLines_previousX=x;
                    rotateLines_previousY=y;
                    if(Math.abs(x)<rotateLines_endPosLimit&&rotateLines_PointNums>rotateLines_MinPointNums){
                        mGLRender.UpdateTransformMatrix(0, y*2/screenHeight, 0, 1);
                        rotateLines_hasEnd=true;
                    }
                    requestRender();
                }
            }
            return true;
        }


        if(!display_deformation){//视图模式
            if (e.getPointerCount() == 1) {

                long currentTimeMillis = System.currentTimeMillis();
                if (currentTimeMillis - mLastMultiTouchTime > 200)
                {
                    if(!rotate_translation){//旋转模式
                        float y = e.getY();
                        float x = e.getX();
                        switch (e.getAction()) {
                            case MotionEvent.ACTION_MOVE:
                                float dy = y - mPreviousY_Angle;
                                float dx = x - mPreviousX_Angle;
                                mYAngle += dx * TOUCH_SCALE_FACTOR;
                                mXAngle += dy * TOUCH_SCALE_FACTOR;
                        }
                        mPreviousY_Angle = y;
                        mPreviousX_Angle = x;
                        mGLRender.UpdateTransformMatrix(mXAngle, mYAngle, mCurScale, mCurScale);
                        requestRender();
                    }
                    else{//位移模式
                        float y = e.getY();
                        float x = e.getX();
                        switch (e.getAction()) {
                            case MotionEvent.ACTION_MOVE:
                                float dy = y - mPreviousY_Offset;
                                float dx = x - mPreviousX_Offset;
                                mXOffset += dx * TOUCH_DISPLACEMENT_FACTOR;
                                mYOffset += dy * TOUCH_DISPLACEMENT_FACTOR;
                        }
                        mPreviousY_Offset = y;
                        mPreviousX_Offset = x;
                        mGLRender.UpdateTransformMatrix(mXOffset, -mYOffset, mCurScale, mCurScale);
                        requestRender();
                    }
                }
            } else
                mScaleGestureDetector.onTouchEvent(e);
        }
        else{//变形模式
            if (e.getPointerCount() == 1){
                if(!ffd_dffd){
                        switch (e.getAction()) {
                            case MotionEvent.ACTION_MOVE:
                                mGLRender.UpdateTransformMatrix(e.getX(), e.getY(), 1, 0);
                                break;
                            case MotionEvent.ACTION_DOWN://手指第一次触及屏幕，选择控制顶点
                                mGLRender.UpdateTransformMatrix(e.getX(), e.getY(), 0, 1);
                                break;
                            case MotionEvent.ACTION_UP://手指离开屏幕，放弃控制顶点
                                mGLRender.UpdateTransformMatrix(e.getX(), e.getY(), 0, 0);
                                break;
                        }
                        requestRender();
                }
                else{//dffd模式
                    final float y = e.getY();
                    final float x = e.getX();
                    switch (e.getAction()) {
                        case MotionEvent.ACTION_DOWN:
                            clickCount++;
                            handler.postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    if (clickCount == 1) {
                                        //单击
                                        System.out.println("dffd单击！");
                                        mGLRender.UpdateTransformMatrix(x, y, 0, 0);
                                    }else if(clickCount==2){
                                        //双击
                                        System.out.println("dffd双击！");
                                        mGLRender.UpdateTransformMatrix(x, y, 0, 1);
                                        requestRender();
                                    }
                                    handler.removeCallbacksAndMessages(null);
                                    //清空handler延时，并防内存泄漏
                                    clickCount = 0;//计数清零
                                }
                            },timeout);//延时timeout后执行run方法中的代码
                        break;
                        case MotionEvent.ACTION_MOVE:
                            mGLRender.UpdateTransformMatrix(x, y, 1, 0);
                            requestRender();
                            break;
                        case MotionEvent.ACTION_UP:
                            mGLRender.UpdateTransformMatrix(x, y, 1, 1);
                            break;
                    }
                }
            }
        }

        return true;
    }
    //设置控件大小
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        //屏幕旋转导致需要交换
        screenWidth=width;
        screenHeight=height;

        if (0 == mRatioWidth || 0 == mRatioHeight) {
            setMeasuredDimension(width, height);
        } else {
            if (width < height * mRatioWidth / mRatioHeight) {
                setMeasuredDimension(width, width * mRatioHeight / mRatioWidth);
            } else {
                setMeasuredDimension(height * mRatioWidth / mRatioHeight, height);
            }
        }
    }

    public void setAspectRatio(int width, int height) {
        Log.d(TAG, "setAspectRatio() called with: width = [" + width + "], height = [" + height + "]");
        if (width < 0 || height < 0) {
            throw new IllegalArgumentException("Size cannot be negative.");
        }

        mRatioWidth = width;
        mRatioHeight = height;
        System.out.println("屏幕宽度="+mRatioWidth);
        System.out.println("屏幕高度="+mRatioHeight);
        requestLayout();
    }

    public MyGLRender getGLRender() {
        return mGLRender;
    }
    //缩放手势
    @Override
    public boolean onScale(ScaleGestureDetector detector) {

        if(!display_deformation){
            float preSpan = detector.getPreviousSpan();
            float curSpan = detector.getCurrentSpan();
            mCurScale= (preSpan-curSpan) / 6000;
            if(rotate_translation)
                mGLRender.UpdateTransformMatrix(mXOffset, mYOffset, mCurScale, mCurScale);
            else
                mGLRender.UpdateTransformMatrix(mXAngle, mYAngle, mCurScale, mCurScale);

            mCurScale=0;
            requestRender();
        }
        return false;
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        //mPreScale = mCurScale;
        mLastMultiTouchTime = System.currentTimeMillis();
    }


    public static class MyGLRender implements GLSurfaceView.Renderer {
        private MyNativeRender mNativeRender;
        private int mSampleType;

        MyGLRender() {
            mNativeRender = new MyNativeRender();
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            mNativeRender.native_OnSurfaceCreated();

        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            mNativeRender.native_OnSurfaceChanged(width, height);

        }

        @Override
        public void onDrawFrame(GL10 gl) {
            mNativeRender.native_OnDrawFrame();

        }

        public void Init() {
            mNativeRender.native_Init();
        }

        public void UnInit() {
            mNativeRender.native_UnInit();
        }

        public void SetParamsInt(int paramType, int value0, int value1) {
            if (paramType == SAMPLE_TYPE) {
                mSampleType = value0;
            }
            mNativeRender.native_SetParamsInt(paramType, value0, value1);
        }
        public void recoverConTrolPoints(){
            mNativeRender.native_RecoverConTrolPoints();
        }
        public void SetImageData(int format, int width, int height, byte[] bytes) {
            mNativeRender.native_SetImageData(format, width, height, bytes);
        }

        public void SetImageDataWithIndex(int index, int format, int width, int height, byte[] bytes) {
            mNativeRender.native_SetImageDataWithIndex(index, format, width, height, bytes);
        }

        public int getSampleType() {
            return mSampleType;
        }

        /***
         *
         * @param rotateX
         * @param rotateY
         * @param scaleX 用于变形时，代表是否移动中
         * @param scaleY 用于变形时，代表是否按下/是否双击
         */
        public void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
        {
            mNativeRender.native_UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
        }

    }
}
