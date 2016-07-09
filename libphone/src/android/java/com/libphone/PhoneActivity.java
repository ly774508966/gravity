package com.libphone;

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseArray;
import android.os.Handler;
import android.view.ViewTreeObserver;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;
import android.view.animation.AlphaAnimation;
import android.widget.AbsoluteLayout;
import android.view.ViewGroup;
import android.view.View;
import android.content.Context;
import android.widget.TextView;
import com.libphone.PhoneNotifyThread;
import android.view.animation.Animation;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.HashMap;
import android.widget.AbsListView;
import android.view.animation.Animation.AnimationListener;
import android.graphics.drawable.Drawable;
import android.widget.EditText;
import android.text.InputType;
import android.view.inputmethod.InputMethodManager;
import android.text.TextWatcher;
import android.text.Editable;
import android.view.MotionEvent;
import android.content.res.Configuration;
import android.view.Gravity;
import android.graphics.drawable.shapes.RoundRectShape;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.Paint;
import android.graphics.drawable.shapes.Shape;
import android.graphics.Canvas;
import android.widget.ListView;
import android.graphics.Color;
import android.widget.BaseAdapter;
import android.graphics.drawable.ColorDrawable;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap;
import android.graphics.RectF;
import android.os.Build;
import android.graphics.BitmapShader;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Shader.TileMode;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Window;
import android.view.WindowManager;
import android.annotation.TargetApi;
import android.view.KeyEvent;
import android.widget.AbsListView.OnScrollListener;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.opengl.GLSurfaceView;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;
import java.util.LinkedList;
import java.util.List;
import android.content.res.AssetManager;
import android.hardware.SensorEventListener;
import android.hardware.SensorEvent;
import android.util.FloatMath;

public class PhoneActivity extends Activity {

    class PhoneContainerBackgroundDrawable extends ShapeDrawable {
        private Paint fillPaint = null;
        private Paint strokePaint = null;
        private float cornerRadius = 0;

        @Override
        public void onDraw(Shape shape, Canvas canvas, Paint paint) {
            if (0 == cornerRadius) {
                shape.draw(canvas, fillPaint);
            } else {
                RectF rect = new RectF(0, 0, shape.getWidth(), shape.getHeight());
                canvas.drawRoundRect(rect, cornerRadius, cornerRadius, fillPaint);
            }
            if (null != strokePaint) {
                shape.draw(canvas, strokePaint);
            }
        }

        private void createStrokePaintIfNeed(int borderWidth, int borderColor) {
            if (borderWidth > 0 && 0 != borderColor) {
                strokePaint = new Paint(fillPaint);
                strokePaint.setStyle(Paint.Style.STROKE);
                strokePaint.setColor(borderColor);
                strokePaint.setStrokeWidth(borderWidth);
            }
        }

        public PhoneContainerBackgroundDrawable(Shape shape, int borderWidth, int borderColor,
                                                int backgroundColor, float radius) {
            super(shape);
            fillPaint = this.getPaint();
            fillPaint.setColor(backgroundColor);
            cornerRadius = radius;
            createStrokePaintIfNeed(borderWidth, borderColor);
        }

        public PhoneContainerBackgroundDrawable(Shape shape, int borderWidth, int borderColor,
                                                Bitmap backgroundBitmap, float radius) {
            super(shape);
            fillPaint = this.getPaint();
            fillPaint.setAntiAlias(true);
            fillPaint.setShader(new BitmapShader(backgroundBitmap,
                    Shader.TileMode.CLAMP, Shader.TileMode.CLAMP));
            cornerRadius = radius;
            createStrokePaintIfNeed(borderWidth, borderColor);
        }
    }

    class PhoneViewShadow {
        private float shadowRadius = 0;
        private float shadowOffsetX = 0;
        private float shadowOffsetY = 0;
        private int shadowColor = 0;
        private float shadowOpacity = 0;
        private Paint shadowPaint = null;
        private float insetSize = 0;

        public Paint getPaint() {
            return shadowPaint;
        }

        public float getInsetSize() {
            return insetSize;
        }

        public void setInsetSize(float size) {
            insetSize = size;
        }

        public void setColor(int color) {
            shadowColor = color;
            apply();
        }

        public void setOffset(float offsetX, float offsetY) {
            shadowOffsetX = offsetX;
            shadowOffsetY = offsetY;
            apply();
        }

        public void setOpacity(float opacity) {
            shadowOpacity = opacity;
            apply();
        }

        public void setRadius(float radius) {
            shadowRadius = radius;
            apply();
        }

        private void apply() {
            shadowPaint = new Paint();
            shadowPaint.setShadowLayer(shadowRadius, shadowOffsetX, shadowOffsetY,
                    0xff000000 | shadowColor);
            shadowPaint.setStyle(Paint.Style.FILL);
            shadowPaint.setColor(0xff000000 | shadowColor);
            shadowPaint.setStrokeCap(Paint.Cap.ROUND);
        }
    }

    class PhoneContainerView extends AbsoluteLayout {
        private float cornerRadius = 0;
        private float borderWidth = 0;
        private int borderColor = 0;
        private int backgroundColor = 0;
        private int backgroundImageResourceId = 0;
        private String backgroundImagePath = null;
        private SparseArray<PhoneViewShadow> childShadowMap = null;

        public PhoneContainerView(Context context) {
            super(context);
        }

        @Override
        protected void onDraw (Canvas canvas) {
            if (null != childShadowMap) {
                for(int i = 0; i < childShadowMap.size(); i++) {
                    int childHandle = childShadowMap.keyAt(i);
                    PhoneViewShadow shadow = (PhoneViewShadow)childShadowMap.valueAt(i);
                    View view = (View)findHandleObject(childHandle);
                    float insetSize = shadow.getInsetSize();
                    canvas.drawRect(view.getLeft() + insetSize, view.getTop() + insetSize,
                            view.getWidth() + view.getLeft() - insetSize - insetSize,
                            view.getHeight() + view.getTop() - insetSize - insetSize,
                            shadow.getPaint());
                }
            }
            super.onDraw(canvas);
        }

        public PhoneViewShadow findChildShadow(int childHandle) {
            if (null == childShadowMap) {
                return null;
            }
            return childShadowMap.get(childHandle);
        }

        public void addChildShadow(int childHandle, PhoneViewShadow shadow) {
            if (null == childShadowMap) {
                childShadowMap = new SparseArray<PhoneViewShadow>();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
                    setLayerType(LAYER_TYPE_SOFTWARE, null);
                }
            }
            childShadowMap.put(childHandle, shadow);
        }

        public void setCornerRadius(float radius) {
            cornerRadius = radius;
            applyBackgroud();
        }

        public void setBorderWidth(float width) {
            borderWidth = width;
            applyBackgroud();
        }

        public void setBorderColor(int color) {
            borderColor = color;
            applyBackgroud();
        }

        public void setBackgroundImageResourceId(int resId) {
            backgroundImageResourceId = resId;
            backgroundImagePath = null;
            applyBackgroud();
        }

        public void setBackgroundFillColor(int color) {
            backgroundColor = color;
            applyBackgroud();
        }

        public void setBackgroundImagePath(String path) {
            backgroundImagePath = path;
            backgroundImageResourceId = 0;
            applyBackgroud();
        }

        private PhoneViewShadow createShadowIfNotExist() {
            PhoneContainerView parent = (PhoneContainerView)getParent();
            PhoneViewShadow shadow = parent.findChildShadow(getId());
            if (null == shadow) {
                shadow = new PhoneViewShadow();
                shadow.setInsetSize(PhoneActivity.this.dp(2));
                parent.addChildShadow(getId(), shadow);
                parent.invalidate();
            }
            return shadow;
        }

        public void setShadowColor(int color) {
            createShadowIfNotExist().setColor(color);
        }

        public void setShadowOffset(float offsetX, float offsetY) {
            createShadowIfNotExist().setOffset(offsetX, offsetY);
        }

        public void setShadowOpacity(float opacity) {
            createShadowIfNotExist().setOpacity(opacity);
        }

        public void setShadowRadius(float radius) {
            createShadowIfNotExist().setRadius(radius);
        }

        private boolean shouldUseDefaultBackground() {
            return (0 == cornerRadius && 0 == borderWidth && 0 == borderColor) ||
                    0 == getWidth() || 0 == getHeight();
        }

        @Override
        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            if (!shouldUseDefaultBackground()) {
                applyBackgroud();
            }
        }

        private void applyBackgroud() {
            if (shouldUseDefaultBackground()) {
                if (null != backgroundImagePath) {
                    try {
                        setBackgroundDrawable(Drawable.createFromPath(backgroundImagePath));
                    } catch (Throwable t) {
                    }
                } else if (0 != backgroundImageResourceId) {
                    setBackgroundResource(backgroundImageResourceId);
                } else if (0 != backgroundColor) {
                    setBackgroundColor(backgroundColor);
                }
            } else {
                RoundRectShape shape = new RoundRectShape(
                        new float[]{cornerRadius, cornerRadius, cornerRadius, cornerRadius,
                                cornerRadius, cornerRadius, cornerRadius, cornerRadius},
                        null,
                        null);
                PhoneContainerBackgroundDrawable drawable = null;
                if (null != backgroundImagePath) {
                    drawable = new PhoneContainerBackgroundDrawable(shape,
                            (int)borderWidth, borderColor,
                            Bitmap.createScaledBitmap(BitmapFactory.decodeFile(backgroundImagePath),
                                    getWidth(), getHeight(), true),
                            cornerRadius);
                } else if (0 != backgroundImageResourceId) {
                    drawable = new PhoneContainerBackgroundDrawable(shape,
                            (int)borderWidth, borderColor,
                            Bitmap.createScaledBitmap(BitmapFactory.decodeResource(getResources(),
                                    backgroundImageResourceId), getWidth(), getHeight(), true),
                            cornerRadius);
                } else {
                    drawable = new PhoneContainerBackgroundDrawable(shape,
                            (int)borderWidth, borderColor, backgroundColor,
                            cornerRadius);
                }
                if (null != drawable) {
                    setBackgroundDrawable(drawable);
                }
            }
        }
    }

    public class SectionRow {
        public int section;
        public int row;
    }

    public class PhoneTableCellView extends PhoneContainerView {
        public View customView = null;
        public PhoneTableCellView(Context context) {
            super(context);
        }
        public void setCustomView(View view) {
            if (customView == view) {
                return;
            }
            if (null != customView && this == customView.getParent()) {
                this.removeView(customView);
            }
            customView = view;
            if (null != customView && this != customView.getParent()) {
                ((ViewGroup)customView.getParent()).removeView(customView);
                this.addView(customView);
            }
        }
        public View getRenderView() {
            if (null != customView) {
                return customView;
            }
            return null;
        }
    }

    public class PhoneTableViewAdapter extends BaseAdapter {
        public ListView listView = null;
        public int handle = 0;
        public boolean isGrouped = false;
        private SparseArray<Integer> sectionRowNumMap = new SparseArray<Integer>();
        private HashMap<String, Integer> viewTypeMap = new HashMap<String, Integer>();
        private int totalRecordsNum = 0;
        private int sectionCount = 0;

        public PhoneTableViewAdapter(ListView target, boolean grouped) {
            this.listView = target;
            this.handle = target.getId();
            this.isGrouped = grouped;
        }

        public int sectionRowToPosition(int section, int row) {
            int position = 0;
            int sectionIndex = 0;
            for (sectionIndex = 0; sectionIndex <= section; ++sectionIndex) {
                position += sectionRowNumMap.get(sectionIndex);
            }
            return position;
        }

        public SectionRow positionToSectionRow(int position) {
            SectionRow item = new SectionRow();
            int count = 0;
            int sectionIndex = 0;
            int lastSectionIndex = 0;
            int lastRowIndex = 0;
            int lastCount = 0;
            for (sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) {
                lastSectionIndex = sectionIndex;
                lastCount = count;
                count += sectionRowNumMap.get(sectionIndex);
                if (count >= position) {
                    lastRowIndex = position - lastCount;
                    break;
                }
            }
            item.section = lastSectionIndex;
            item.row = lastRowIndex;
            return item;
        }

        public void rebuildSectionRowNumMap() {
            int section;
            int total = 0;
            sectionCount = nativeRequestTableViewSectionCount(handle);
            for (section = 0; section < sectionCount; ++section) {
                int rowNum = nativeRequestTableViewRowCount(handle, section);
                sectionRowNumMap.put(section, rowNum);
                total += rowNum;
            }
            totalRecordsNum = total;
            notifyDataSetChanged();
        }

        @Override
        public boolean hasStableIds () {
            return false;
        }

        @Override
        public int getCount() {
            return totalRecordsNum;
        }

        @Override
        public int getViewTypeCount() {
            return nativeRequestTableViewCellIdentifierTypeCount(handle);
        }

        @Override
        public int getItemViewType(int position) {
            SectionRow item = positionToSectionRow(position);
            String identifier = nativeRequestTableViewCellIdentifier(handle, item.section, item.row);
            Integer id = viewTypeMap.get(identifier);
            if (null == id) {
                int newIdVal = viewTypeMap.size();
                viewTypeMap.put(identifier, newIdVal);
                return newIdVal;
            }
            return id;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            PhoneTableCellView cellView;
            int customHandle;
            SectionRow item = positionToSectionRow(position);
            if (null == convertView || !(convertView instanceof PhoneTableCellView)) {
                cellView = new PhoneTableCellView(PhoneActivity.this);
                convertView = cellView;
            } else {
                cellView = (PhoneTableCellView)convertView;
            }
            customHandle = nativeRequestTableViewCellCustomView(handle, item.section, item.row);
            if (0 != customHandle) {
                cellView.setCustomView((View)findHandleObject(customHandle));
            }
            cellView.setLayoutParams(
                    new AbsListView.LayoutParams(listView.getWidth(),
                            nativeRequestTableViewRowHeight(handle, item.section, item.row)));
            nativeRequestTableViewCellRender(handle, item.section, item.row,
                    cellView.getRenderView().getId());
            return convertView;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return 0;
        }
    }

    public class PhoneTableView extends ListView {
        public boolean isGrouped = false;
        private PhoneTableViewAdapter adapter = null;
        private float lastMotionY = 0;
        private PhoneContainerView refreshContainerView = null;
        final int refreshStableHeight = (int)PhoneActivity.this.dp(60);
        private boolean isRefreshing = false;
        private boolean needShowRefreshView = false;

        public PhoneTableView(Context context) {
            super(context);
            setDivider(null);
            setDividerHeight(0);
            setPadding(0, 0, 0, 0);
            setCacheColorHint(Color.TRANSPARENT);
            setScrollingCacheEnabled(false);
            setSelector(new ColorDrawable(Color.TRANSPARENT));
        }

        public void reload() {
            if (null == adapter) {
                if (null == refreshContainerView) {
                    int handle = getId();
                    int refreshHandle = nativeRequestTableViewRefreshView(handle);
                    if (0 != refreshHandle) {
                        View view = (View)findHandleObject(refreshHandle);
                        refreshContainerView =
                                new PhoneContainerView(PhoneActivity.this);
                        refreshContainerView.setBackgroundColor(0xff000000 | 0xaaaaaa);
                        refreshContainerView.setLayoutParams(new AbsListView.LayoutParams(getWidth(),
                                (int)0));
                        refreshContainerView.setId(refreshHandle);
                        ((ViewGroup)view.getParent()).removeView(view);
                        refreshContainerView.addView(view);
                        view.setVisibility(View.GONE);
                        needShowRefreshView = true;
                        addHeaderView(refreshContainerView);
                    }
                }
                adapter = new PhoneTableViewAdapter(this, isGrouped);
                setAdapter(adapter);
            }
            adapter.rebuildSectionRowNumMap();
        }

        private void resetRefreshToHidden() {
            if (null != refreshContainerView) {
                refreshContainerView.setLayoutParams(new AbsListView.LayoutParams(getWidth(),
                        (int) 0));
                nativeRequestTableViewUpdateRefreshView(getId(),
                        refreshContainerView.getId());
                lastMotionY = 0;
                ((View)findHandleObject(refreshContainerView.getId())).
                        setVisibility(View.GONE);
                needShowRefreshView = true;
            }
        }

        private void resetRefreshToStable() {
            if (null != refreshContainerView) {
                if (refreshContainerView.getHeight() != refreshStableHeight) {
                    refreshContainerView.setLayoutParams(new AbsListView.LayoutParams(getWidth(),
                            (int) refreshStableHeight));
                    nativeRequestTableViewUpdateRefreshView(getId(),
                            refreshContainerView.getId());
                    lastMotionY = 0;
                }
            }
        }

        public void beginRefreshing() {
            if (!isRefreshing) {
                isRefreshing = true;
                resetRefreshToStable();
            }
        }

        public void endRefreshing() {
            if (isRefreshing) {
                if (null != refreshContainerView) {
                    resetRefreshToHidden();
                }
                isRefreshing = false;
            }
        }

        public float getRefreshHeight() {
            if (null != refreshContainerView) {
                return refreshContainerView.getHeight();
            }
            return 0;
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (null != refreshContainerView)
            switch (event.getAction()) {
                case MotionEvent.ACTION_UP:
                    if (isRefreshing) {
                        resetRefreshToStable();
                    } else {
                        resetRefreshToHidden();
                    }
                    break;
                case MotionEvent.ACTION_DOWN:
                    lastMotionY = event.getY();
                    break;
                case MotionEvent.ACTION_MOVE:
                    if (0 == lastMotionY) {
                        lastMotionY = event.getY();
                    }
                    if (0 == getFirstVisiblePosition()) {
                        if (null != refreshContainerView) {
                            boolean needNotify = false;
                            int height = (int) ((event.getY() - lastMotionY) / 1.7);
                            if (height < 0) {
                                height = 0;
                            }
                            if (height > refreshContainerView.getHeight()) {
                                if (needShowRefreshView) {
                                    needShowRefreshView = false;
                                    ((View)findHandleObject(refreshContainerView.getId())).
                                            setVisibility(View.VISIBLE);
                                }
                                if (height > refreshStableHeight + refreshStableHeight / 2) {
                                    if (!isRefreshing) {
                                        needNotify = true;
                                    }
                                }
                            }
                            if (needNotify) {
                                isRefreshing = true;
                                nativeRequestTableViewRefresh(getId());
                            }
                            if (isRefreshing) {
                                height += refreshStableHeight;
                            }
                            nativeRequestTableViewUpdateRefreshView(getId(),
                                    refreshContainerView.getId());
                            refreshContainerView.setLayoutParams(new AbsListView.LayoutParams(getWidth(),
                                    height));
                        }
                    } else {
                        lastMotionY = event.getY();
                    }
                    break;
            }
            return super.onTouchEvent(event);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    private PhoneContainerView container;
    private boolean lunched = false;
    private int lastWindowWidth = 0;
    private int lastWindowHeight = 0;
    private List<GLSurfaceView> openGLViewList = null;

    private void addOpenGLViewToList(GLSurfaceView view) {
        if (null == openGLViewList) {
            openGLViewList = new LinkedList<GLSurfaceView>();
        }
        openGLViewList.add(view);
    }

    private void removeOpenGLViewFromList(GLSurfaceView view) {
        openGLViewList.remove(view);
    }

    private void pauseAllOpenGLViews() {
        if (null != openGLViewList) {
            Iterator<GLSurfaceView> iterator = openGLViewList.iterator();
            while (iterator.hasNext()) {
                iterator.next().onPause();
            }
        }
    }

    private void resumeAllOpenGLViews() {
        if (null != openGLViewList) {
            Iterator<GLSurfaceView> iterator = openGLViewList.iterator();
            while (iterator.hasNext()) {
                iterator.next().onResume();
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        container = new PhoneContainerView(this);
        container.setId(0);
        setContentView(container);
        ViewTreeObserver viewTreeObserver = container.getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                if (!lunched) {
                    lunched = true;
                    lastWindowWidth = container.getWidth();
                    lastWindowHeight = container.getHeight();
                    lunchWithNative();
                } else {
                    if (lastWindowWidth != container.getWidth() ||
                            lastWindowHeight != container.getHeight()) {
                        lastWindowWidth = container.getWidth();
                        lastWindowHeight = container.getHeight();
                        nativeSendAppLayoutChanging();
                    }
                }
            }
        });
    }

    public void onStart() {
        super.onStart();
    }

    public void onResume() {
        if (lunched) {
            nativeSendAppShowing();
        }
        super.onResume();
        resumeAllOpenGLViews();
    }

    public void onRestart() {
        super.onRestart();
    }

    public void onPause() {
        pauseAllOpenGLViews();
        if (lunched) {
            nativeSendAppHiding();
        }
        super.onPause();
    }

    public void onStop() {
        super.onStop();
    }

    public void onDestroy() {
        if (lunched) {
            nativeSendAppTerminating();
        }
        handler.postDelayed(new Runnable() {
            public void run() {
                android.os.Process.killProcess(android.os.Process.myPid());
                System.exit(0);
            }
        }, 0);
        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_BACK){
            if (1 == nativeSendAppBackClick()) {
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    private void lunchWithNative() {
        nativeInit();
        nativeInitDensity(getResources().getDisplayMetrics().density);
        assetManager = getResources().getAssets();
        nativeInitAssetManager(assetManager);

        nativeSendAppShowing();

        PhoneNotifyThread notifyThread = new PhoneNotifyThread();
        notifyThread.handler = handler;
        notifyThread.activity = PhoneActivity.this;
        notifyThread.start();
    }

    public float dp(float size) {
        return getResources().getDisplayMetrics().density * size;
    }

    public class PhoneNotifyRunnable implements Runnable {
        public long needNotifyMask;
        @Override
        public void run() {
            PhoneActivity.this.nativeNotifyMainThread(needNotifyMask);
        }
    }

    private SparseArray<Object> handleMap = new SparseArray<Object>();
    private Handler handler = new Handler();
    private AssetManager assetManager = null;

    private native int nativeSendAppShowing();
    private native int nativeSendAppHiding();
    private native int nativeSendAppTerminating();
    private native int nativeInvokeTimer(int handle);
    private native int nativeNotifyMainThread(long needNotifyMask);
    private native int nativeAnimationFinished(int handle);
    private native int nativeDispatchViewTouchBeginEvent(int handle, int x, int y);
    private native int nativeDispatchViewTouchEndEvent(int handle, int x, int y);
    private native int nativeDispatchViewTouchMoveEvent(int handle, int x, int y);
    private native int nativeDispatchViewTouchCancelEvent(int handle, int x, int y);
    private native int nativeInitDensity(float density);
    private native int nativeInit();
    private native int nativeInitAssetManager(AssetManager assetManager);
    private native int nativeRequestTableViewCellCustomView(int handle, int section, int row);
    private native String nativeRequestTableViewCellIdentifier(int handle, int section, int row);
    private native int nativeRequestTableViewSectionCount(int handle);
    private native int nativeRequestTableViewRowCount(int handle, int section);
    private native int nativeRequestTableViewRowHeight(int handle, int section, int row);
    private native int nativeRequestTableViewCellIdentifierTypeCount(int handle);
    private native int nativeRequestTableViewCellRender(int handle, int section, int row, int renderHandle);
    private native int nativeSendAppBackClick();
    private native int nativeRequestTableViewRefresh(int handle);
    private native int nativeRequestTableViewUpdateRefreshView(int handle, int renderHandle);
    private native int nativeRequestTableViewRefreshView(int handle);
    private native int nativeSendAppLayoutChanging();
    private native int nativeInvokeOpenGLViewRender(int handle, long func);
    private native int nativeInvokeThread(int handle, long func);
    private native int nativeDispatchShake();

    private Object findHandleObject(int handle) {
        return handleMap.get(handle);
    }

    private void setHandleObject(int handle, Object obj) {
        handleMap.put(handle, obj);
    }

    private void removeHandleObject(int handle) {
        handleMap.remove(handle);
    }

    public class PhoneTimerRunnable implements Runnable {
        public long period;
        public int handle;
        public boolean removed = false;
        public void run() {
            if (removed) {
                return;
            }
            nativeInvokeTimer(handle);
            if (!removed) {
                handler.postDelayed(this, period);
            }
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    public int javaCreateTimer(int handle, long milliseconds) {
        PhoneTimerRunnable timerRunnable = new PhoneTimerRunnable();
        timerRunnable.period = milliseconds;
        timerRunnable.handle = handle;
        setHandleObject(handle, timerRunnable);
        handler.postDelayed(timerRunnable, milliseconds);
        return 0;
    }

    public int javaRemoveTimer(int handle) {
        PhoneTimerRunnable timerRunnable = (PhoneTimerRunnable)findHandleObject(handle);
        timerRunnable.removed = true;
        removeHandleObject(handle);
        return 0;
    }

    public int javaCreateContainerView(int handle, int parentHandle) {
        PhoneContainerView view = new PhoneContainerView(this);
        setHandleObject(handle, view);
        view.setId(handle);
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaSetViewFrame(int handle, float x, float y, float width, float height) {
        if (0 == handle) {
            return -1;
        }
        View view = (View)findHandleObject(handle);
        view.setLayoutParams(new AbsoluteLayout.LayoutParams((int)width, (int)height, (int)x,
                (int)y));
        return 0;
    }

    public int javaSetViewBackgroundColor(int handle, int color) {
        if (0 == handle) {
            container.setBackgroundColor(color | 0xff000000);
            return 0;
        }
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setBackgroundFillColor(color | 0xff000000);
        return 0;
    }

    public int javaSetViewFontColor(int handle, int color) {
        TextView view = (TextView)findHandleObject(handle);
        view.setTextColor(color | 0xff000000);
        return 0;
    }

    private void addViewToParent(View view, int parentHandle) {
        if (0 != parentHandle) {
            ((ViewGroup)findHandleObject(parentHandle)).addView(view);
        } else {
            container.addView(view);
        }
    }

    public int javaCreateTextView(int handle, int parentHandle) {
        TextView view = new TextView(this);
        setHandleObject(handle, view);
        view.setId(handle);
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaSetViewText(int handle, String val) {
        TextView view = (TextView)findHandleObject(handle);
        view.setText(val);
        return 0;
    }

    public int javaShowView(int handle, int display) {
        View view = (View)findHandleObject(handle);
        view.setVisibility(0 == display ? View.GONE : View.VISIBLE);
        return 0;
    }

    public float javaGetViewWidth(int handle) {
        if (0 == handle) {
            return container.getWidth();
        }
        return ((View)findHandleObject(handle)).getWidth();
    }

    public float javaGetViewHeight(int handle) {
        if (0 == handle) {
            return container.getHeight();
        }
        return ((View)findHandleObject(handle)).getHeight();
    }

    public int javaCreateViewAnimationSet(int handle) {
        ArrayList<Integer> animationSet = new ArrayList<Integer>();
        setHandleObject(handle, animationSet);
        return 0;
    }

    public int javaAddViewAnimationToSet(int animationHandle, int setHanle) {
        ArrayList<Integer> set = (ArrayList<Integer>)findHandleObject(setHanle);
        set.add(animationHandle);
        return 0;
    }

    public int javaRemoveViewAnimationSet(int handle) {
        removeHandleObject(handle);
        return 0;
    }

    public int javaRemoveViewAnimation(int handle) {
        removeHandleObject(handle);
        return 0;
    }

    public class PhoneAnimationPair {
        public Animation animation;
        public View view;
    }

    public int javaCreateViewTranslateAnimation(int handle, int viewHandle,
                                                 float offsetX, float offsetY) {
        PhoneAnimationPair pair = new PhoneAnimationPair();
        TranslateAnimation ani = new TranslateAnimation(0, offsetX, 0, offsetY);
        View view = (View)findHandleObject(viewHandle);
        ani.setFillAfter(true);
        ani.setFillBefore(false);
        final int finalHandle = handle;
        final int finalViewHandle = viewHandle;
        final float finalOffsetX = offsetX;
        final float finalOffsetY = offsetY;
        ani.setAnimationListener(new AnimationListener(){
            public void onAnimationStart(Animation ani) {
            };
            public void onAnimationRepeat(Animation ani) {
            };
            public void onAnimationEnd(Animation ani) {
                View view = (View)findHandleObject(finalViewHandle);
                view.setLayoutParams(new AbsoluteLayout.LayoutParams((int)view.getWidth(),
                        (int)view.getHeight(), (int)(view.getLeft() + finalOffsetX),
                        (int)(view.getTop() + finalOffsetY)));
                view.clearAnimation();
                nativeAnimationFinished(finalHandle);
            };
        });
        pair.animation = ani;
        pair.view = view;
        setHandleObject(handle, pair);
        return 0;
    }

    public int javaCreateViewAlphaAnimation(int handle, int viewHandle,
                                            float fromAlpha, float toAlpha) {
        PhoneAnimationPair pair = new PhoneAnimationPair();
        AlphaAnimation ani = new AlphaAnimation(fromAlpha, toAlpha);
        View view = (View)findHandleObject(viewHandle);
        ani.setFillAfter(true);
        final int finalHandle = handle;
        ani.setAnimationListener(new AnimationListener(){
            public void onAnimationStart(Animation ani) {
            };
            public void onAnimationRepeat(Animation ani) {
            };
            public void onAnimationEnd(Animation ani) {
                nativeAnimationFinished(finalHandle);
            };
        });
        pair.animation = ani;
        pair.view = view;
        setHandleObject(handle, pair);
        return 0;
    }

    public int javaBeginAnimationSet(int handle, int duration) {
        ArrayList<Integer> list = (ArrayList<Integer>)findHandleObject(handle);
        for (Iterator<Integer> it = list.iterator(); it.hasNext();) {
            int animationHandle = (int)it.next();
            PhoneAnimationPair pair = (PhoneAnimationPair)findHandleObject(animationHandle);
            pair.animation.setDuration(duration);
            pair.view.startAnimation(pair.animation);
        }
        return 0;
    }

    public int javaBringViewToFront(int handle) {
        View view = (View)findHandleObject(handle);
        view.bringToFront();
        return 0;
    }

    public int javaSetViewAlpha(int handle, float alpha) {
        View view = (View)findHandleObject(handle);
        AlphaAnimation ani = new AlphaAnimation(alpha, alpha);
        ani.setFillAfter(true);
        ani.setDuration(0);
        view.startAnimation(ani);
        return 0;
    }

    public int javaSetViewFontSize(int handle, float fontSize) {
        TextView view = (TextView)findHandleObject(handle);
        view.setTextSize(TypedValue.COMPLEX_UNIT_PX, fontSize * (float)0.75);
        return 0;
    }

    public int javaSetViewBackgroundImageResource(int handle, String imageResource) {
        int dotPos = imageResource.indexOf('.');
        String name = -1 == dotPos ? imageResource : imageResource.substring(0, dotPos);
        int resId = getResources().getIdentifier(name,
                "drawable",
                getPackageName());
        if (0 == handle) {
            container.setBackgroundResource(resId);
        } else {
            PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
            view.setBackgroundImageResourceId(resId);
        }
        return 0;
    }

    public int javaSetViewBackgroundImagePath(int handle, String imagePath) {
        if (0 == handle) {
            try {
                container.setBackgroundDrawable(Drawable.createFromPath(imagePath));
            } catch (Throwable t) {
            }
        } else {
            PhoneContainerView view = (PhoneContainerView) findHandleObject(handle);
            view.setBackgroundImagePath(imagePath);
        }
        return 0;
    }

    public int javaCreateEditTextView(int handle, int parentHandle) {
        EditText view = new EditText(this);
        setHandleObject(handle, view);
        view.setId(handle);
        view.setPadding(0, 0, 0, 0);
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaShowSoftInputOnView(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        imm.toggleSoftInput(0, InputMethodManager.HIDE_NOT_ALWAYS);
        return 0;
    }

    public int javaHideSoftInputOnView(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        return 0;
    }

    public String javaGetViewText(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        return (String)view.getText();
    }

    public int javaSetViewInputTypeAsVisiblePassword(int handle) {
        EditText view = (EditText)findHandleObject(handle);
        view.setInputType(InputType.TYPE_CLASS_TEXT |
                InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        return 0;
    }

    public int javaSetViewInputTypeAsPassword(int handle) {
        EditText view = (EditText)findHandleObject(handle);
        view.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        return 0;
    }

    public int javaSetViewInputTypeAsText(int handle) {
        EditText view = (EditText)findHandleObject(handle);
        view.setInputType(InputType.TYPE_CLASS_TEXT);
        return 0;
    }

    public native int nativeDispatchViewClickEvent(int handle);
    public native int nativeDispatchViewLongClickEvent(int handle);
    public native int nativeDispatchViewValueChangeEvent(int handle);

    public int javaEnableViewClickEvent(int handle) {
        View view = (View)findHandleObject(handle);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PhoneActivity.this.nativeDispatchViewClickEvent(v.getId());
            }
        });
        return 0;
    }

    public int javaEnableViewLongClickEvent(int handle) {
        View view = (View)findHandleObject(handle);
        view.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                int result = PhoneActivity.this.nativeDispatchViewLongClickEvent(v.getId());
                if (1 == result) {
                    return true;
                }
                return false;
            }
        });
        return 0;
    }

    public int javaEnableViewValueChangeEvent(int handle) {
        View unknownTypeView = (View)findHandleObject(handle);
        String className = unknownTypeView.getClass().getName();
        if (className.equals("android.widget.EditText")) {
            EditText view = (EditText)unknownTypeView;
            final int finalHandle = handle;
            view.addTextChangedListener(new TextWatcher() {
                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                }
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }
                @Override
                public void afterTextChanged(Editable s) {
                    PhoneActivity.this.nativeDispatchViewValueChangeEvent(finalHandle);
                }
            });
        }
        return 0;
    }

    public int javaEnableViewTouchEvent(int handle) {
        View view = (View)findHandleObject(handle);
        view.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        return 1 == nativeDispatchViewTouchBeginEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                    case MotionEvent.ACTION_UP:
                        return 1 == nativeDispatchViewTouchEndEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                    case MotionEvent.ACTION_MOVE:
                        return 1 == nativeDispatchViewTouchMoveEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                    case MotionEvent.ACTION_CANCEL:
                        return 1 == nativeDispatchViewTouchCancelEvent(v.getId(),
                                (int)event.getX(), (int)event.getY());
                }
                return false;
            }
        });
        return 0;
    }

    public int javaIsLandscape() {
        return Configuration.ORIENTATION_LANDSCAPE ==
                getResources().getConfiguration().orientation ? 1 : 0;
    }

    public int javaSetViewAlignCenter(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        view.setGravity(Gravity.CENTER);
        return 0;
    }

    public int javaSetViewAlignLeft(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        view.setGravity(Gravity.LEFT);
        return 0;
    }

    public int javaSetViewAlignRight(int handle) {
        TextView view = (TextView)findHandleObject(handle);
        view.setGravity(Gravity.RIGHT);
        return 0;
    }

    public int javaSetViewCornerRadius(int handle, float radius) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setCornerRadius(radius);
        return 0;
    }

    public int javaSetViewBorderColor(int handle, int color) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setBorderColor(color | 0xff000000);
        return 0;
    }

    public int javaSetViewBorderWidth(int handle, float width) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setBorderWidth(width);
        return 0;
    }

    public int javaCreateTableView(int grouped, int handle, int parentHandle) {
        PhoneTableView view = new PhoneTableView(this);
        setHandleObject(handle, view);
        view.setId(handle);
        view.isGrouped = 1 == grouped;
        addViewToParent(view, parentHandle);
        return 0;
    }

    public int javaReloadTableView(int handle) {
        PhoneTableView view = (PhoneTableView)findHandleObject(handle);
        view.reload();
        return 0;
    }

    public int javaSetViewShadowColor(int handle, int color) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setShadowColor(color | 0xff000000);
        return 0;
    }

    public int javaSetViewShadowOffset(int handle, float offsetX, float offsetY) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setShadowOffset(offsetX, offsetY);
        return 0;
    }

    public int javaSetViewShadowOpacity(int handle, float opacity) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setShadowOpacity(opacity);
        return 0;
    }

    public int javaSetViewShadowRadius(int handle, float radius) {
        PhoneContainerView view = (PhoneContainerView)findHandleObject(handle);
        view.setShadowRadius(radius);
        return 0;
    }

    public int javaSetViewBackgroundImageRepeat(int handle, int repeat) {
        View view = (View)findHandleObject(handle);
        if (null != view) {
            BitmapDrawable drawable = (BitmapDrawable)view.getBackground();
            if (null != drawable) {
                if (1 == repeat) {
                    drawable.setTileModeXY(TileMode.REPEAT, TileMode.REPEAT);
                }
            }
        }
        return 0;
    }

    public int javaSetViewFontBold(int handle, int bold) {
        TextView view = (TextView)findHandleObject(handle);
        if (1 == bold) {
            view.setTypeface(Typeface.DEFAULT_BOLD);
        } else {
            view.setTypeface(Typeface.DEFAULT);
        }
        return 0;
    }

    public int javaSetStatusBarBackgroundColor(int color) {
        if (Build.VERSION.SDK_INT >= 21) {
            Window window = getWindow();
            window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            window.addFlags(0x80000000);
            //window.setStatusBarColor(0xff000000 | color);
            try {
                window.getClass().getDeclaredMethod("setStatusBarColor", int.class).invoke(window,
                        0xff000000 | color);
            } catch (Exception e) {
                e.printStackTrace();
            }
            return 0;
        };
        return -1;
    }

    public int javaRotateView(int handle, float degree) {
        View view = (View)findHandleObject(handle);
        RotateAnimation ani = new RotateAnimation(degree, degree, view.getWidth() / 2,
                view.getHeight() / 2);
        ani.setFillAfter(true);
        ani.setDuration(0);
        ani.setRepeatCount(0);
        ani.setRepeatMode(Animation.REVERSE);
        view.startAnimation(ani);
        return 0;
    }

    public float javaGetTableViewRefreshHeight(int handle) {
        PhoneTableView view = (PhoneTableView)findHandleObject(handle);
        return view.getRefreshHeight();
    }

    public int javaBeginTableViewRefresh(int handle) {
        PhoneTableView view = (PhoneTableView)findHandleObject(handle);
        view.beginRefreshing();
        return 0;
    }

    public int javaEndTableViewRefresh(int handle) {
        PhoneTableView view = (PhoneTableView)findHandleObject(handle);
        view.endRefreshing();
        return 0;
    }

    public int javaSetEditTextViewPlaceholder(int handle, String text, int color) {
        EditText view = (EditText)findHandleObject(handle);
        view.setHint(text);
        view.setHintTextColor(0xff000000 | color);
        return 0;
    }

    public int javaSetViewParent(int handle, int parentHandle) {
        View view = (View)findHandleObject(handle);
        ViewGroup newParentView = (ViewGroup)findHandleObject(parentHandle);
        if (null != view.getParent()) {
            ((ViewGroup)view.getParent()).removeView(view);
        }
        newParentView.addView(view);
        return 0;
    }

    public int javaRemoveView(int handle) {
        View view = (View)findHandleObject(handle);
        if (null != view.getParent()) {
            ((ViewGroup)view.getParent()).removeView(view);
        }
        removeHandleObject(handle);
        if (view instanceof GLSurfaceView) {
            removeOpenGLViewFromList((GLSurfaceView) view);
        }
        return 0;
    }

    public int javaCreateOpenGLView(int handle, int parentHandle) {
        GLSurfaceView view = new GLSurfaceView(this);
        setHandleObject(handle, view);
        view.setId(handle);
        addViewToParent(view, parentHandle);
        view.setEGLContextClientVersion(2);
        view.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        addOpenGLViewToList(view);
        return 0;
    }

    public class PhoneOpenGLViewRender implements GLSurfaceView.Renderer {
        private int invokeHandle = 0;
        private long invokeFunc = 0;

        public PhoneOpenGLViewRender(int handle, long func) {
            invokeHandle = handle;
            invokeFunc = func;
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            // No-op
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            nativeInvokeOpenGLViewRender(invokeHandle, invokeFunc);
        }
    }

    public int javaBeginOpenGLViewRender(int handle, long func) {
        GLSurfaceView view = (GLSurfaceView)findHandleObject(handle);
        view.setRenderer(new PhoneOpenGLViewRender(handle, func));
        return 0;
    }

    class PhoneThread extends Thread {
        public int handle;
        public long func;
        public void run() {
            nativeInvokeThread(handle, func);
        }
    }

    public int javaCreateThread(int handle, String name) {
        PhoneThread thread = new PhoneThread();
        thread.setName(name);
        setHandleObject(handle, thread);
        return 0;
    }

    public int javaStartThread(int handle, long func) {
        PhoneThread thread = (PhoneThread)findHandleObject(handle);
        thread.handle = handle;
        thread.func = func;
        thread.start();
        return 0;
    }

    public int javaJoinThread(int handle) {
        PhoneThread thread = (PhoneThread)findHandleObject(handle);
        try {
            thread.join();
            return 0;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return -1;
    }

    public int javaRemoveThread(int handle) {
        removeHandleObject(handle);
        return 0;
    }

    public int javaIsShakeDetectionSupported() {
        SensorManager sensorManager = (SensorManager)getSystemService(SENSOR_SERVICE);
        return null != sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER) ? 1 : 0;
    }

    private SensorEventListener shakeEventListener = null;
    private float shakeAccelerometer = 0;
    private float shakeLastAccelerometer = 0;
    private float shakeCurrentAccelerometer = 0;
    private long shakeLastTime = 0;
    private long shakeRegisterTime = 0;

    public int javaStartShakeDetection() {
        if (null == shakeEventListener) {
            shakeRegisterTime = System.currentTimeMillis();
            shakeEventListener = new SensorEventListener() {
                @Override
                public void onAccuracyChanged(Sensor sensor, int accuracy) {
                    // ignore
                }

                @Override
                public void onSensorChanged(SensorEvent event) {
                    float x = event.values[0];
                    float y = event.values[1];
                    float z = event.values[2];
                    long currentTime = System.currentTimeMillis();
                    shakeLastAccelerometer = shakeCurrentAccelerometer;
                    shakeCurrentAccelerometer = (float)Math.sqrt(x*x + y*y + z*z);
                    float delta = shakeCurrentAccelerometer - shakeLastAccelerometer;
                    shakeAccelerometer = shakeAccelerometer * 0.9f + delta;
                    if (shakeAccelerometer > 5) {
                        if (shakeLastTime > 0 && currentTime - shakeLastTime < 500 &&
                                currentTime - shakeLastTime > 60) {
                            if (currentTime - shakeRegisterTime > 500) {
                                nativeDispatchShake();
                            }
                        }
                        shakeLastTime = System.currentTimeMillis();
                    }
                }
            };
            SensorManager sensorManager = (SensorManager)getSystemService(SENSOR_SERVICE);
            Sensor sensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            sensorManager.registerListener(shakeEventListener, sensor,
                    SensorManager.SENSOR_DELAY_UI);
        }
        return 0;
    }

    public int javaStopShakeDetection() {
        SensorManager sensorManager = (SensorManager)getSystemService(SENSOR_SERVICE);
        sensorManager.unregisterListener(shakeEventListener);
        shakeEventListener = null;
        return 0;
    }

    public int javaGetViewParent(int handle) {
        View view = (View)findHandleObject(handle);
        View parentView = (View)view.getParent();
        return parentView.getId();
    }
}
