package com.gc.app;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

import static android.opengl.GLSurfaceView.RENDERMODE_WHEN_DIRTY;
import static com.gc.app.MyGLSurfaceView.IMAGE_FORMAT_NV21;
import static com.gc.app.MyGLSurfaceView.IMAGE_FORMAT_RGBA;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private static final String[] REQUEST_PERMISSIONS = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
    };
    private static final int PERMISSION_REQUEST_CODE = 1;
     //后续可改为变形模式等
    private static final String[] SAMPLE_TITLES = {
            "设定旋转面曲线","变形体式自由变形","直接自由变形"
    };

    private MyGLSurfaceView mGLSurfaceView;
    private ViewGroup mRootView;
    private int mSampleSelectedIndex = 0;
    private Switch display_Switch,manipulation_Switch,deformation_view_Switch,showBoundbox_Switch;
    private Button recoverTranformation_Button;

    //操作方式，将以上两个bool用int传入
    private int system_mode;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mRootView = (ViewGroup) findViewById(R.id.rootView);
        mGLSurfaceView = (MyGLSurfaceView) findViewById(R.id.my_gl_surface_view);
        mGLSurfaceView.getGLRender().Init();
        mGLSurfaceView.setRenderMode(RENDERMODE_WHEN_DIRTY);
        display_Switch=(Switch)findViewById(R.id.displayMode_switch);
        display_Switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean ischecked) {
                mGLSurfaceView.wireFrame_fill=(ischecked?0:1);
                mGLSurfaceView.reSetRotateLine();
                mGLSurfaceView.getGLRender().SetParamsInt(CommonUtils.cal_system_mode(mGLSurfaceView.display_deformation,mGLSurfaceView.rotate_translation,mGLSurfaceView.ffd_dffd,mGLSurfaceView.setLine_deformation), mGLSurfaceView.showBoundbox, mGLSurfaceView.wireFrame_fill);
                mGLSurfaceView.requestRender();
            }
        });

        manipulation_Switch=(Switch)findViewById(R.id.manipulationMode_switch);
        manipulation_Switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean ischecked) {
                mGLSurfaceView.rotate_translation=ischecked;
                mGLSurfaceView.reSetRotateLine();
                mGLSurfaceView.getGLRender().SetParamsInt(CommonUtils.cal_system_mode(mGLSurfaceView.display_deformation,mGLSurfaceView.rotate_translation,mGLSurfaceView.ffd_dffd,mGLSurfaceView.setLine_deformation), mGLSurfaceView.showBoundbox, mGLSurfaceView.wireFrame_fill);
                mGLSurfaceView.requestRender();
            }
        });

        deformation_view_Switch=(Switch)findViewById(R.id.deformation_view_switch);
        deformation_view_Switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean ischecked) {
                mGLSurfaceView.display_deformation=ischecked;
                mGLSurfaceView.reSetRotateLine();
                mGLSurfaceView.getGLRender().SetParamsInt(CommonUtils.cal_system_mode(mGLSurfaceView.display_deformation,mGLSurfaceView.rotate_translation,mGLSurfaceView.ffd_dffd,mGLSurfaceView.setLine_deformation), mGLSurfaceView.showBoundbox, mGLSurfaceView.wireFrame_fill);
                mGLSurfaceView.requestRender();
            }
        });

        showBoundbox_Switch=(Switch)findViewById(R.id.showBoundbox_switch);
        showBoundbox_Switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean ischecked) {
                mGLSurfaceView.showBoundbox=(ischecked?1:0);
                mGLSurfaceView.reSetRotateLine();
                mGLSurfaceView.getGLRender().SetParamsInt(CommonUtils.cal_system_mode(mGLSurfaceView.display_deformation,mGLSurfaceView.rotate_translation,mGLSurfaceView.ffd_dffd,mGLSurfaceView.setLine_deformation), mGLSurfaceView.showBoundbox, mGLSurfaceView.wireFrame_fill);
                mGLSurfaceView.requestRender();
            }
        });

        recoverTranformation_Button=(Button)findViewById(R.id.deformationReset_button);
        recoverTranformation_Button.setOnClickListener(new View.OnClickListener(){

            @Override
            public void onClick(View view) {
                if(!mGLSurfaceView.setLine_deformation){
                    if(mGLSurfaceView.rotateLines_hasEnd){
                        mGLSurfaceView.setLine_deformation=true;
                        recoverTranformation_Button.setText("复位");
                        mGLSurfaceView.getGLRender().recoverConTrolPoints();
                        mGLSurfaceView.requestRender();
                    }
                }
                else{
                    mGLSurfaceView.getGLRender().recoverConTrolPoints();
                    mGLSurfaceView.requestRender();
                }
            }
        });
    }



    @Override
    protected void onResume() {
        super.onResume();
        if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
            ActivityCompat.requestPermissions(this, REQUEST_PERMISSIONS, PERMISSION_REQUEST_CODE);
        }
        //把资源assert拷贝到sdcard中，用于加载
        //CommonUtils.copyAssetsDirToSDCard(MainActivity.this, "heart", "/sdcard/model");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
                Toast.makeText(this, "We need the permission: WRITE_EXTERNAL_STORAGE", Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mGLSurfaceView.getGLRender().UnInit();

        /*
        * Once the EGL context gets destroyed all the GL buffers etc will get destroyed with it,
        * so this is unnecessary.
        * */
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_change_sample) {
            showGLSampleDialog();
        }
        return true;
    }

    private void showGLSampleDialog() {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = LayoutInflater.from(this);
        final View rootView = inflater.inflate(R.layout.sample_selected_layout, null);

        final AlertDialog dialog = builder.create();

        Button confirmBtn = rootView.findViewById(R.id.confirm_btn);
        confirmBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.cancel();
            }
        });

        final RecyclerView resolutionsListView = rootView.findViewById(R.id.resolution_list_view);

        final MyRecyclerViewAdapter myPreviewSizeViewAdapter = new MyRecyclerViewAdapter(this, Arrays.asList(SAMPLE_TITLES));
        myPreviewSizeViewAdapter.setSelectIndex(mSampleSelectedIndex);
        myPreviewSizeViewAdapter.addOnItemClickListener(new MyRecyclerViewAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(View view, int position) {

                int selectIndex = myPreviewSizeViewAdapter.getSelectIndex();
                myPreviewSizeViewAdapter.setSelectIndex(position);
                myPreviewSizeViewAdapter.notifyItemChanged(selectIndex);
                myPreviewSizeViewAdapter.notifyItemChanged(position);
                mSampleSelectedIndex = position;

                mGLSurfaceView.setRenderMode(RENDERMODE_WHEN_DIRTY);
/*
                if (mRootView.getWidth() != mGLSurfaceView.getWidth()
                        || mRootView.getHeight() != mGLSurfaceView.getHeight()) {
                    mGLSurfaceView.setAspectRatio(mRootView.getWidth(), mRootView.getHeight());
                }
                */
                mGLSurfaceView.ffd_dffd=(mSampleSelectedIndex==2);
                mGLSurfaceView.reSetRotateLine();
                //比较特殊，为了以后改进，这里点击右上方菜单允许重新开始绘制旋转线阶段
                //mGLSurfaceView.setLine_deformation=false;
                mGLSurfaceView.getGLRender().SetParamsInt(CommonUtils.cal_system_mode(mGLSurfaceView.display_deformation,mGLSurfaceView.rotate_translation,mGLSurfaceView.ffd_dffd,mGLSurfaceView.setLine_deformation), mGLSurfaceView.showBoundbox, mGLSurfaceView.wireFrame_fill);
                mGLSurfaceView.requestRender();
                dialog.cancel();

            }
        });

        LinearLayoutManager manager = new LinearLayoutManager(this);
        manager.setOrientation(LinearLayoutManager.VERTICAL);
        resolutionsListView.setLayoutManager(manager);

        resolutionsListView.setAdapter(myPreviewSizeViewAdapter);
        resolutionsListView.scrollToPosition(mSampleSelectedIndex);

        dialog.show();
        dialog.getWindow().setContentView(rootView);

    }

    private Bitmap LoadRGBAImage(int resId) {
        InputStream is = this.getResources().openRawResource(resId);
        Bitmap bitmap;
        try {
            bitmap = BitmapFactory.decodeStream(is);
            if (bitmap != null) {
                int bytes = bitmap.getByteCount();
                ByteBuffer buf = ByteBuffer.allocate(bytes);
                bitmap.copyPixelsToBuffer(buf);
                byte[] byteArray = buf.array();
                mGLSurfaceView.getGLRender().SetImageData(IMAGE_FORMAT_RGBA, bitmap.getWidth(), bitmap.getHeight(), byteArray);
            }
        }
        finally
        {
            try
            {
                is.close();
            }
            catch(IOException e)
            {
                e.printStackTrace();
            }
        }
        return bitmap;
    }

    private void LoadRGBAImage(int resId, int index) {
        InputStream is = this.getResources().openRawResource(resId);
        Bitmap bitmap;
        try {
            bitmap = BitmapFactory.decodeStream(is);
            if (bitmap != null) {
                int bytes = bitmap.getByteCount();
                ByteBuffer buf = ByteBuffer.allocate(bytes);
                bitmap.copyPixelsToBuffer(buf);
                byte[] byteArray = buf.array();
                mGLSurfaceView.getGLRender().SetImageDataWithIndex(index, IMAGE_FORMAT_RGBA, bitmap.getWidth(), bitmap.getHeight(), byteArray);
            }
        }
        finally
        {
            try
            {
                is.close();
            }
            catch(IOException e)
            {
                e.printStackTrace();
            }
        }
    }

    private void LoadNV21Image() {
        InputStream is = null;
        try {
            is = getAssets().open("YUV_Image_840x1074.NV21");
        } catch (IOException e) {
            e.printStackTrace();
        }

        int lenght = 0;
        try {
            lenght = is.available();
            byte[] buffer = new byte[lenght];
            is.read(buffer);
            mGLSurfaceView.getGLRender().SetImageData(IMAGE_FORMAT_NV21, 840, 1074, buffer);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try
            {
                is.close();
            }
            catch(IOException e)
            {
                e.printStackTrace();
            }
        }

    }

    protected boolean hasPermissionsGranted(String[] permissions) {
        for (String permission : permissions) {
            if (ActivityCompat.checkSelfPermission(this, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    public static class MyRecyclerViewAdapter extends RecyclerView.Adapter<MyRecyclerViewAdapter.MyViewHolder> implements View.OnClickListener {
        private List<String> mTitles;
        private Context mContext;
        private int mSelectIndex = 0;
        private OnItemClickListener mOnItemClickListener = null;

        public MyRecyclerViewAdapter(Context context, List<String> titles) {
            mContext = context;
            mTitles = titles;
        }

        public void setSelectIndex(int index) {
            mSelectIndex = index;
        }

        public int getSelectIndex() {
            return mSelectIndex;
        }

        public void addOnItemClickListener(OnItemClickListener onItemClickListener) {
            mOnItemClickListener = onItemClickListener;
        }

        @NonNull
        @Override
        public MyViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.sample_item_layout, parent, false);
            MyViewHolder myViewHolder = new MyViewHolder(view);
            view.setOnClickListener(this);
            return myViewHolder;
        }

        @Override
        public void onBindViewHolder(@NonNull MyViewHolder holder, int position) {
            holder.mTitle.setText(mTitles.get(position));
            if (position == mSelectIndex) {
                holder.mRadioButton.setChecked(true);
                holder.mTitle.setTextColor(mContext.getResources().getColor(R.color.colorAccent));
            } else {
                holder.mRadioButton.setChecked(false);
                holder.mTitle.setText(mTitles.get(position));
                holder.mTitle.setTextColor(Color.GRAY);
            }
            holder.itemView.setTag(position);
        }

        @Override
        public int getItemCount() {
            return mTitles.size();
        }

        @Override
        public void onClick(View v) {
            if (mOnItemClickListener != null) {
                mOnItemClickListener.onItemClick(v, (Integer) v.getTag());
            }
        }

        public interface OnItemClickListener {
            void onItemClick(View view, int position);
        }

        class MyViewHolder extends RecyclerView.ViewHolder {
            RadioButton mRadioButton;
            TextView mTitle;

            public MyViewHolder(View itemView) {
                super(itemView);
                mRadioButton = itemView.findViewById(R.id.radio_btn);
                mTitle = itemView.findViewById(R.id.item_title);
            }
        }
    }

}
