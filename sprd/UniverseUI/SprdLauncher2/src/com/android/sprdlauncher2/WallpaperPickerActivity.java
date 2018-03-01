/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.sprdlauncher2;

import android.animation.Animator;
import android.animation.LayoutTransition;
import android.annotation.TargetApi;
import android.app.ActionBar;
import android.app.Activity;
import android.app.WallpaperInfo;
import android.app.WallpaperManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.DataSetObserver;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LevelListDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.util.Pair;
import android.view.ActionMode;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.accessibility.AccessibilityEvent;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.webkit.MimeTypeMap;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ProgressBar;

/* SPRD: bug304635 2014-04-23 prompt user if load bitmap error. @{ */
import android.widget.Toast;
/* SPRD: bug304635 2014-04-23 prompt user if load bitmap error. @} */

import com.android.gallery3d.exif.ExifInterface;
import com.android.photos.BitmapRegionTileSource;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;


/* SPRD: Fix bug 267426 @{ */
import android.os.Handler;
/* @} */

public class WallpaperPickerActivity extends WallpaperCropActivity {
    static final String TAG = "Launcher.WallpaperPickerActivity";

    public static final int IMAGE_PICK = 5;
    public static final int PICK_WALLPAPER_THIRD_PARTY_ACTIVITY = 6;
    public static final int PICK_LIVE_WALLPAPER = 7;
    private static final String TEMP_WALLPAPER_TILES = "TEMP_WALLPAPER_TILES";

    /* SPRD: Fix bug 264823 @{ */
    public static final int SDK_VERSION_HONEYCOMB_MR2 = 13;
    public static final String KEY_SCALE = "scale";
    public static final String KEY_OUTPUT_X = "outputX";
    public static final String KEY_OUTPUT_Y = "outputY";
    public static final String KEY_ASPECT_X = "aspectX";
    public static final String KEY_ASPECT_Y = "aspectY";
    public static final String KEY_SPOTLIGHT_X = "spotlightX";
    public static final String KEY_SPOTLIGHT_Y = "spotlightY";
    public static final String KEY_SET_AS_WALLPAPER = "set-as-wallpaper";
    public static final String KEY_SCALE_UP_IF_NEEDED = "scaleUpIfNeeded";
    public static final String GALLERY_PKG_NAME = "com.android.gallery3d";
    public static final String CROP_ACTIVITY_ACTION = "com.android.camera.action.CROP";
    public static final String CROP_ACTIVITY_CLASS = "com.android.gallery3d.filtershow.crop.CropActivity";
    /* @} */

    /* SPRD: Fix bug 267426 @{ */
    private Bitmap mlastPhoto = null;
    /* @} */

    /* SPRD: Fix bug328822 ,add flag for apply DRM files. @{ */
    public static final String KEY_PICK_IMAGE_FOR_WALLPAPER = "applyForWallpaper";
    /* @} */

    private View mSelectedThumb;
    private boolean mIgnoreNextTap;
    private OnClickListener mThumbnailOnClickListener;

    private LinearLayout mWallpapersView;
    private View mWallpaperStrip;

    private ActionMode.Callback mActionModeCallback;
    private ActionMode mActionMode;

    private View.OnLongClickListener mLongClickListener;

    ArrayList<Uri> mTempWallpaperTiles = new ArrayList<Uri>();
    private SavedWallpaperImages mSavedImages;
    private WallpaperInfo mLiveWallpaperInfoOnPickerLaunch;

    public static abstract class WallpaperTileInfo {
        protected View mView;
        public void setView(View v) {
            mView = v;
        }
        public void onClick(WallpaperPickerActivity a) {}
        public void onSave(WallpaperPickerActivity a) {}
        public void onDelete(WallpaperPickerActivity a) {}
        public boolean isSelectable() { return false; }
        public boolean isNamelessWallpaper() { return false; }
        public void onIndexUpdated(CharSequence label) {
            if (isNamelessWallpaper()) {
                mView.setContentDescription(label);
            }
        }
    }

    public static class PickImageInfo extends WallpaperTileInfo {
        @Override
        public void onClick(WallpaperPickerActivity a) {
            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.setType("image/*");
            // SPRD: Fix bug287820 ,add flag for apply DRM files.
            intent.putExtra(KEY_PICK_IMAGE_FOR_WALLPAPER, true);
            Utilities.startActivityForResultSafely(a, intent, IMAGE_PICK);
        }
    }

    public static class UriWallpaperInfo extends WallpaperTileInfo {
        private Uri mUri;
        public UriWallpaperInfo(Uri uri) {
            mUri = uri;
        }
        @Override
        public void onClick(WallpaperPickerActivity a) {
            CropView v = a.getCropView();
            int rotation = WallpaperCropActivity.getRotationFromExif(a, mUri);
            /* SPRD: Fix bug 328822, do scale to adapter screen. @{ */
            v.setTileSource(new BitmapRegionTileSource(a, mUri, 1024, rotation), null, false);
            /* @} */
            v.setTouchEnabled(true);
        }
        @Override
        public void onSave(final WallpaperPickerActivity a) {
            boolean finishActivityWhenDone = true;
            OnBitmapCroppedHandler h = new OnBitmapCroppedHandler() {
                public void onBitmapCropped(byte[] imageBytes) {
                    Point thumbSize = getDefaultThumbnailSize(a.getResources());
                    // rotation is set to 0 since imageBytes has already been correctly rotated
                    Bitmap thumb = createThumbnail(
                            thumbSize, null, null, imageBytes, null, 0, 0, true);
                    a.getSavedImages().writeImage(thumb, imageBytes);
                }
            };
            a.cropImageAndSetWallpaper(mUri, h, finishActivityWhenDone);
        }
        @Override
        public boolean isSelectable() {
            return true;
        }
        @Override
        public boolean isNamelessWallpaper() {
            return true;
        }
    }

    public static class ResourceWallpaperInfo extends WallpaperTileInfo {
        private Resources mResources;
        private int mResId;
        private Drawable mThumb;

        public ResourceWallpaperInfo(Resources res, int resId, Drawable thumb) {
            mResources = res;
            mResId = resId;
            mThumb = thumb;
        }
        @Override
        public void onClick(WallpaperPickerActivity a) {
            int rotation = WallpaperCropActivity.getRotationFromExif(mResources, mResId);
            BitmapRegionTileSource source = new BitmapRegionTileSource(
                    mResources, a, mResId, 1024, rotation);
            CropView v = a.getCropView();
            /* SPRD: Fix bug 262347, do scale to adapter screen. @{ */
            v.setTileSource(source, null, false);
            /* @} */
            Point wallpaperSize = WallpaperCropActivity.getDefaultWallpaperSize(
                    a.getResources(), a.getWindowManager());
            RectF crop = WallpaperCropActivity.getMaxCropRect(
                    source.getImageWidth(), source.getImageHeight(),
                    wallpaperSize.x, wallpaperSize.y, false);
            v.setScale(wallpaperSize.x / crop.width());
            v.setTouchEnabled(false);
        }
        @Override
        public void onSave(WallpaperPickerActivity a) {
            boolean finishActivityWhenDone = true;
            a.cropImageAndSetWallpaper(mResources, mResId, finishActivityWhenDone);
        }
        @Override
        public boolean isSelectable() {
            return true;
        }
        @Override
        public boolean isNamelessWallpaper() {
            return true;
        }
    }

    public void setWallpaperStripYOffset(float offset) {
        mWallpaperStrip.setPadding(0, 0, 0, (int) offset);
    }

    // called by onCreate; this is subclassed to overwrite WallpaperCropActivity
    protected void init() {
        setContentView(R.layout.wallpaper_picker);

        mCropView = (CropView) findViewById(R.id.cropView);
        mWallpaperStrip = findViewById(R.id.wallpaper_strip);
        mCropView.setTouchCallback(new CropView.TouchCallback() {
            LauncherViewPropertyAnimator mAnim;
            @Override
            public void onTouchDown() {
                if (mAnim != null) {
                    mAnim.cancel();
                }
                if (mWallpaperStrip.getAlpha() == 1f) {
                    mIgnoreNextTap = true;
                }
                mAnim = new LauncherViewPropertyAnimator(mWallpaperStrip);
                /* SPRD: Fix bug 267311 @{ */
                int visibility = mWallpaperStrip.getVisibility();
                if (visibility == View.VISIBLE) {
                    mAnim.alpha(0f)
                    .setDuration(150)
                    .addListener(new Animator.AnimatorListener() {
                        public void onAnimationStart(Animator animator) { }
                        public void onAnimationEnd(Animator animator) {
                            mWallpaperStrip.setVisibility(View.INVISIBLE);
                        }
                        public void onAnimationCancel(Animator animator) { }
                        public void onAnimationRepeat(Animator animator) { }
                    });
                    mAnim.setInterpolator(new AccelerateInterpolator(0.75f));
                } else {
                    mWallpaperStrip.setVisibility(View.VISIBLE);
                    mAnim.alpha(1f)
                    .setDuration(150)
                    .setInterpolator(new DecelerateInterpolator(0.75f));
                    // SPRD: Fix bug 310344, wallpaper strip view have not show.
                    mWallpaperStrip.requestLayout();
                }
                /* @} */
                mAnim.start();
            }
            @Override
            public void onTouchUp() {
                mIgnoreNextTap = false;
            }
            @Override
            public void onTap() {
                /* SPRD: Fix bug 267311 @{ */
                /*
                boolean ignoreTap = mIgnoreNextTap;
                mIgnoreNextTap = false;
                if (!ignoreTap) {
                    if (mAnim != null) {
                        mAnim.cancel();
                    }
                    mWallpaperStrip.setVisibility(View.VISIBLE);
                    mAnim = new LauncherViewPropertyAnimator(mWallpaperStrip);
                    mAnim.alpha(1f)
                         .setDuration(150)
                         .setInterpolator(new DecelerateInterpolator(0.75f));
                    mAnim.start();
                }
                */
                /* @} */
            }
        });

        mThumbnailOnClickListener = new OnClickListener() {
            public void onClick(View v) {
                if (mActionMode != null) {
                    // When CAB is up, clicking toggles the item instead
                    if (v.isLongClickable()) {
                        mLongClickListener.onLongClick(v);
                    }
                    return;
                }
                WallpaperTileInfo info = (WallpaperTileInfo) v.getTag();
                if (info.isSelectable()) {
                    if (mSelectedThumb != null) {

                        /* SPRD: bug277489 2014-02-09 sync selected state with checked state @{ */
                        if (mSelectedThumb instanceof CheckableFrameLayout) {
                            ((CheckableFrameLayout) mSelectedThumb).setChecked(false);
                        }
                        /* SPRD: bug277489 2014-02-09 sync selected state with checked state @} */

                        mSelectedThumb.setSelected(false);
                        mSelectedThumb = null;
                    }
                    mSelectedThumb = v;
                    v.setSelected(true);
                    // TODO: Remove this once the accessibility framework and
                    // services have better support for selection state.
                    v.announceForAccessibility(
                            getString(R.string.announce_selection, v.getContentDescription()));
                }
                info.onClick(WallpaperPickerActivity.this);
            }
        };
        mLongClickListener = new View.OnLongClickListener() {
            // Called when the user long-clicks on someView
            public boolean onLongClick(View view) {
                CheckableFrameLayout c = (CheckableFrameLayout) view;
                c.toggle();

                if (mActionMode != null) {
                    mActionMode.invalidate();
                } else {
                    // Start the CAB using the ActionMode.Callback defined below
                    mActionMode = startActionMode(mActionModeCallback);
                    int childCount = mWallpapersView.getChildCount();
                    for (int i = 0; i < childCount; i++) {
                        mWallpapersView.getChildAt(i).setSelected(false);
                    }
                }
                return true;
            }
        };

        mWallpapersView = (LinearLayout) findViewById(R.id.wallpaper_list);
        initWallpaperListAsync();

        // Action bar
        // Show the custom action bar view
        final ActionBar actionBar = getActionBar();
        actionBar.setCustomView(R.layout.actionbar_set_wallpaper);
        actionBar.getCustomView().setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (mSelectedThumb != null) {
                            WallpaperTileInfo info = (WallpaperTileInfo) mSelectedThumb.getTag();
                            info.onSave(WallpaperPickerActivity.this);

                            /* SPRD: bug269754 2014-01-20 quick and multi-click @{
                             * "save wallpaper" action item cause unexpected behavior */
                            // here we don't need the mSelectedThumb any more, because
                            // this Activity will finish after we click the "save wallpaper" action bar item.
                            // we set mSelectedThumb to null to not allow click event run multi-times.
                            mSelectedThumb = null;
                            /* SPRD: bug269754 2014-01-20 quick and multi-click @} */

                        }
                    }
                });

        // CAB for deleting items
        mActionModeCallback = new ActionMode.Callback() {
            // Called when the action mode is created; startActionMode() was called
            @Override
            public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                // Inflate a menu resource providing context menu items
                MenuInflater inflater = mode.getMenuInflater();
                inflater.inflate(R.menu.cab_delete_wallpapers, menu);
                return true;
            }

            private int numCheckedItems() {
                int childCount = mWallpapersView.getChildCount();
                int numCheckedItems = 0;
                for (int i = 0; i < childCount; i++) {
                    CheckableFrameLayout c = (CheckableFrameLayout) mWallpapersView.getChildAt(i);
                    if (c.isChecked()) {
                        numCheckedItems++;
                    }
                }
                return numCheckedItems;
            }

            // Called each time the action mode is shown. Always called after onCreateActionMode,
            // but may be called multiple times if the mode is invalidated.
            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                int numCheckedItems = numCheckedItems();
                if (numCheckedItems == 0) {
                    mode.finish();
                    return true;
                } else {
                    mode.setTitle(getResources().getQuantityString(
                            R.plurals.number_of_items_selected, numCheckedItems, numCheckedItems));
                    return true;
                }
            }

            // Called when the user selects a contextual menu item
            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                int itemId = item.getItemId();
                if (itemId == R.id.menu_delete) {
                    int childCount = mWallpapersView.getChildCount();
                    ArrayList<View> viewsToRemove = new ArrayList<View>();
                    for (int i = 0; i < childCount; i++) {
                        CheckableFrameLayout c =
                                (CheckableFrameLayout) mWallpapersView.getChildAt(i);
                        if (c.isChecked()) {
                            WallpaperTileInfo info = (WallpaperTileInfo) c.getTag();
                            info.onDelete(WallpaperPickerActivity.this);
                            viewsToRemove.add(c);
                        }
                    }
                    for (View v : viewsToRemove) {
                        mWallpapersView.removeView(v);
                    }
                    updateTileIndices();

                    /* SPRD: bug282258 2014-02-22 wrong checked state with the thumb view @{ */
                    selectNewItem();
                    /* SPRD: bug282258 2014-02-22 wrong checked state with the thumb view @} */

                    mode.finish(); // Action picked, so close the CAB
                    return true;
                } else {
                    return false;
                }
            }

            // Called when the user exits the action mode
            @Override
            public void onDestroyActionMode(ActionMode mode) {
                int childCount = mWallpapersView.getChildCount();
                for (int i = 0; i < childCount; i++) {
                    CheckableFrameLayout c = (CheckableFrameLayout) mWallpapersView.getChildAt(i);
                    c.setChecked(false);
                }
                mSelectedThumb.setSelected(true);
                mActionMode = null;
            }
        };
    }

    private void initializeScrollForRtl() {
        final HorizontalScrollView scroll =
                (HorizontalScrollView) findViewById(R.id.wallpaper_scroll_container);

        if (scroll.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL) {
            final ViewTreeObserver observer = scroll.getViewTreeObserver();
            observer.addOnGlobalLayoutListener(new OnGlobalLayoutListener() {
                public void onGlobalLayout() {
                    LinearLayout masterWallpaperList =
                            (LinearLayout) findViewById(R.id.master_wallpaper_list);
                    scroll.scrollTo(masterWallpaperList.getWidth(), 0);
                    scroll.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                }
            });
        }
    }

    public boolean enableRotation() {
        return super.enableRotation() || Launcher.sForceEnableRotation;
    }

    protected Bitmap getThumbnailOfLastPhoto() {
        Cursor cursor = MediaStore.Images.Media.query(getContentResolver(),
                MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                new String[] { MediaStore.Images.ImageColumns._ID,
                    MediaStore.Images.ImageColumns.DATE_TAKEN},
                null, null, MediaStore.Images.ImageColumns.DATE_TAKEN + " DESC LIMIT 1");
        Bitmap thumb = null;

        /* SPRD: bug316644 2014-05-27 handle unexpected null references. @{ */
        if (cursor != null) {
            if (cursor.moveToNext()) {
                int id = cursor.getInt(0);
                thumb = MediaStore.Images.Thumbnails.getThumbnail(getContentResolver(),
                        id, MediaStore.Images.Thumbnails.MINI_KIND, null);
            }
            cursor.close();
        }
        /* SPRD: bug316644 2014-05-27 handle unexpected null references. @} */

        return thumb;
    }

    protected void onStop() {
        super.onStop();
        mWallpaperStrip = findViewById(R.id.wallpaper_strip);
        if (mWallpaperStrip.getAlpha() < 1f) {
            mWallpaperStrip.setAlpha(1f);
            mWallpaperStrip.setVisibility(View.VISIBLE);
        }
    }

    protected void onSaveInstanceState(Bundle outState) {
        outState.putParcelableArrayList(TEMP_WALLPAPER_TILES, mTempWallpaperTiles);
    }

    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        ArrayList<Uri> uris = savedInstanceState.getParcelableArrayList(TEMP_WALLPAPER_TILES);
        /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @{ */
        // here no need click every stored thumbnail, this will cause low performance and OOM problem,
        // acturely, just click the last one, because only the last one will be show to user.
        if (uris != null) {
            int size = uris.size();
            Uri uri = null;
            for (int i=0; i<size; i++) {
                uri = uris.get(i);
                if (i == (size -1)) {
                    addTemporaryWallpaperTileExt(uri, false, true);
                } else {
                    addTemporaryWallpaperTileExt(uri, false, false);
                }
            }
        }
        /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @} */
    }

    /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @{ */
    private void populateWallpapersFromAdapter(ViewGroup parent, BaseAdapter adapter,
            boolean addLongPressHandler, boolean selectFirstTile) {
        populateWallpapersFromAdapter(parent, adapter, addLongPressHandler, selectFirstTile, false);
    }

    private void populateWallpapersFromAdapter(ViewGroup parent, BaseAdapter adapter,
            boolean addLongPressHandler, boolean selectFirstTile, boolean forceLast) {
        for (int i = 0; i < adapter.getCount(); i++) {
            FrameLayout thumbnail = (FrameLayout) adapter.getView(i, null, parent);

            if (forceLast) {
                parent.addView(thumbnail, parent.getChildCount());
            } else {
                parent.addView(thumbnail, i);
            }

            WallpaperTileInfo info = (WallpaperTileInfo) adapter.getItem(i);
            thumbnail.setTag(info);
            info.setView(thumbnail);
            if (addLongPressHandler) {
                addLongPressHandler(thumbnail);
            }
            thumbnail.setOnClickListener(mThumbnailOnClickListener);
            if (i == 0 && selectFirstTile) {
                mThumbnailOnClickListener.onClick(thumbnail);
            }
        }
    }
    /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @} */

    private void updateTileIndices() {
        LinearLayout masterWallpaperList = (LinearLayout) findViewById(R.id.master_wallpaper_list);
        final int childCount = masterWallpaperList.getChildCount();
        final Resources res = getResources();

        // Do two passes; the first pass gets the total number of tiles
        int numTiles = 0;
        for (int passNum = 0; passNum < 2; passNum++) {
            int tileIndex = 0;
            for (int i = 0; i < childCount; i++) {
                View child = masterWallpaperList.getChildAt(i);
                LinearLayout subList;

                int subListStart;
                int subListEnd;
                if (child.getTag() instanceof WallpaperTileInfo) {
                    subList = masterWallpaperList;
                    subListStart = i;
                    subListEnd = i + 1;
                } else { // if (child instanceof LinearLayout) {
                    subList = (LinearLayout) child;
                    subListStart = 0;
                    subListEnd = subList.getChildCount();
                }

                for (int j = subListStart; j < subListEnd; j++) {
                    WallpaperTileInfo info = (WallpaperTileInfo) subList.getChildAt(j).getTag();
                    if (info.isNamelessWallpaper()) {
                        if (passNum == 0) {
                            numTiles++;
                        } else {
                            CharSequence label = res.getString(
                                    R.string.wallpaper_accessibility_name, ++tileIndex, numTiles);
                            info.onIndexUpdated(label);
                        }
                    }
                }
            }
        }
    }

    private static Point getDefaultThumbnailSize(Resources res) {
        return new Point(res.getDimensionPixelSize(R.dimen.wallpaperThumbnailWidth),
                res.getDimensionPixelSize(R.dimen.wallpaperThumbnailHeight));

    }

    private static Bitmap createThumbnail(Point size, Context context, Uri uri, byte[] imageBytes,
            Resources res, int resId, int rotation, boolean leftAligned) {
        int width = size.x;
        int height = size.y;

        BitmapCropTask cropTask;
        if (uri != null) {
            cropTask = new BitmapCropTask(
                    context, uri, null, rotation, width, height, false, true, null);
        } else if (imageBytes != null) {
            cropTask = new BitmapCropTask(
                    imageBytes, null, rotation, width, height, false, true, null);
        }  else {
            cropTask = new BitmapCropTask(
                    context, res, resId, null, rotation, width, height, false, true, null);
        }
        Point bounds = cropTask.getImageBounds();
        if (bounds == null || bounds.x == 0 || bounds.y == 0) {
            return null;
        }

        Matrix rotateMatrix = new Matrix();
        rotateMatrix.setRotate(rotation);
        float[] rotatedBounds = new float[] { bounds.x, bounds.y };
        rotateMatrix.mapPoints(rotatedBounds);
        rotatedBounds[0] = Math.abs(rotatedBounds[0]);
        rotatedBounds[1] = Math.abs(rotatedBounds[1]);

        RectF cropRect = WallpaperCropActivity.getMaxCropRect(
                (int) rotatedBounds[0], (int) rotatedBounds[1], width, height, leftAligned);
        cropTask.setCropBounds(cropRect);

        if (cropTask.cropBitmap()) {
            return cropTask.getCroppedBitmap();
        } else {
            return null;
        }
    }

    /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @{ */
    /* SPRD: bug304635 2014-04-23 prompt user if load bitmap error. @{ */
    private void addTemporaryWallpaperTile(Uri uri) {
        addTemporaryWallpaperTileExt(uri, true, true);
    }

    private void addTemporaryWallpaperTileExt(Uri uri, boolean prompt, boolean needClick) {
        /* SPRD: bug304678 2014-04-23 read uri error cause empty thumbnail. @{ */
        // Load the thumbnail
        Point defaultSize = getDefaultThumbnailSize(this.getResources());
        int rotation = WallpaperCropActivity.getRotationFromExif(this, uri);
        Bitmap thumb = createThumbnail(defaultSize, this, uri, null, null, 0, rotation, false);
        if (thumb != null) {
            // Add a tile for the image picked from Gallery
            FrameLayout pickedImageThumbnail = (FrameLayout) getLayoutInflater().
                    inflate(R.layout.wallpaper_picker_item, mWallpapersView, false);
            setWallpaperItemPaddingToZero(pickedImageThumbnail);

            ImageView image = (ImageView) pickedImageThumbnail.findViewById(R.id.wallpaper_image);
            image.setImageBitmap(thumb);
            Drawable thumbDrawable = image.getDrawable();
            thumbDrawable.setDither(true);

            mTempWallpaperTiles.add(uri);
            mWallpapersView.addView(pickedImageThumbnail, 0);
            UriWallpaperInfo info = new UriWallpaperInfo(uri);
            pickedImageThumbnail.setTag(info);
            info.setView(pickedImageThumbnail);
            addLongPressHandler(pickedImageThumbnail);
            updateTileIndices();
            pickedImageThumbnail.setOnClickListener(mThumbnailOnClickListener);

            if (needClick) {
                mThumbnailOnClickListener.onClick(pickedImageThumbnail);
            }
        } else {
            Log.e(TAG, "Error loading thumbnail for uri=" + uri);
            if (prompt) {
                Toast.makeText(this, R.string.exif_format_error, Toast.LENGTH_LONG).show();
            }
        }
        /* SPRD: bug304678 2014-04-23 read uri error cause empty thumbnail. @} */
    }
    /* SPRD: bug304635 2014-04-23 prompt user if load bitmap error. @} */
    /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @} */

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == IMAGE_PICK && resultCode == RESULT_OK) {
            if (data != null && data.getData() != null) {
                Uri uri = data.getData();
                handlePickedImageResult(uri);
            }
        } else if (requestCode == PICK_WALLPAPER_THIRD_PARTY_ACTIVITY) {
            setResult(RESULT_OK);
            // SPRD: Fix bug 285141
            //finish();
        } else if (requestCode == PICK_LIVE_WALLPAPER) {
            WallpaperManager wm = WallpaperManager.getInstance(this);
            final WallpaperInfo oldLiveWallpaper = mLiveWallpaperInfoOnPickerLaunch;
            WallpaperInfo newLiveWallpaper = wm.getWallpaperInfo();
            // Try to figure out if a live wallpaper was set;
            if (newLiveWallpaper != null &&
                    (oldLiveWallpaper == null ||
                    !oldLiveWallpaper.getComponent().equals(newLiveWallpaper.getComponent()))) {
                // Return if a live wallpaper was set
                setResult(RESULT_OK);
                finish();
            }
        }
    }

    static void setWallpaperItemPaddingToZero(FrameLayout frameLayout) {
        frameLayout.setPadding(0, 0, 0, 0);
        frameLayout.setForeground(new ZeroPaddingDrawable(frameLayout.getForeground()));
    }

    private void addLongPressHandler(View v) {
        v.setOnLongClickListener(mLongClickListener);
    }

    private ArrayList<ResourceWallpaperInfo> findBundledWallpapers() {
        ArrayList<ResourceWallpaperInfo> bundledWallpapers =
                new ArrayList<ResourceWallpaperInfo>(24);

        Pair<ApplicationInfo, Integer> r = getWallpaperArrayResourceId();
        if (r != null) {
            try {
                Resources wallpaperRes = getPackageManager().getResourcesForApplication(r.first);
                bundledWallpapers = addWallpapers(wallpaperRes, r.first.packageName, r.second);
            } catch (PackageManager.NameNotFoundException e) {
            }
        }

        // Add an entry for the default wallpaper (stored in system resources)
        ResourceWallpaperInfo defaultWallpaperInfo = getDefaultWallpaperInfo();
        if (defaultWallpaperInfo != null) {
            bundledWallpapers.add(0, defaultWallpaperInfo);
        }
        return bundledWallpapers;
    }

    private ResourceWallpaperInfo getDefaultWallpaperInfo() {
        Resources sysRes = Resources.getSystem();
        int resId = sysRes.getIdentifier("default_wallpaper", "drawable", "android");

        File defaultThumbFile = new File(getFilesDir(), "default_thumb.jpg");
        Bitmap thumb = null;
        boolean defaultWallpaperExists = false;
        if (defaultThumbFile.exists()) {
            thumb = BitmapFactory.decodeFile(defaultThumbFile.getAbsolutePath());
            defaultWallpaperExists = true;
        } else {
            Resources res = getResources();
            Point defaultThumbSize = getDefaultThumbnailSize(res);
            int rotation = WallpaperCropActivity.getRotationFromExif(res, resId);
            thumb = createThumbnail(
                    defaultThumbSize, this, null, null, sysRes, resId, rotation, false);
            if (thumb != null) {
                try {
                    defaultThumbFile.createNewFile();
                    FileOutputStream thumbFileStream =
                            openFileOutput(defaultThumbFile.getName(), Context.MODE_PRIVATE);
                    thumb.compress(Bitmap.CompressFormat.JPEG, 95, thumbFileStream);
                    thumbFileStream.close();
                    defaultWallpaperExists = true;
                } catch (IOException e) {
                    Log.e(TAG, "Error while writing default wallpaper thumbnail to file " + e);
                    defaultThumbFile.delete();
                }
            }
        }
        if (defaultWallpaperExists) {
            return new ResourceWallpaperInfo(sysRes, resId, new BitmapDrawable(thumb));
        }
        return null;
    }

    public Pair<ApplicationInfo, Integer> getWallpaperArrayResourceId() {
        // Context.getPackageName() may return the "original" package name,
        // com.android.sprdlauncher2; Resources needs the real package name,
        // com.android.sprdlauncher2. So we ask Resources for what it thinks the
        // package name should be.
        final String packageName = getResources().getResourcePackageName(R.array.wallpapers);
        try {
            ApplicationInfo info = getPackageManager().getApplicationInfo(packageName, 0);
            return new Pair<ApplicationInfo, Integer>(info, R.array.wallpapers);
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }

    private ArrayList<ResourceWallpaperInfo> addWallpapers(
            Resources res, String packageName, int listResId) {
        ArrayList<ResourceWallpaperInfo> bundledWallpapers =
                new ArrayList<ResourceWallpaperInfo>(24);
        final String[] extras = res.getStringArray(listResId);
        for (String extra : extras) {
            int resId = res.getIdentifier(extra, "drawable", packageName);
            if (resId != 0) {
                final int thumbRes = res.getIdentifier(extra + "_small", "drawable", packageName);

                if (thumbRes != 0) {
                    ResourceWallpaperInfo wallpaperInfo =
                            new ResourceWallpaperInfo(res, resId, res.getDrawable(thumbRes));
                    bundledWallpapers.add(wallpaperInfo);
                    // Log.d(TAG, "add: [" + packageName + "]: " + extra + " (" + res + ")");
                }
            } else {
                Log.e(TAG, "Couldn't find wallpaper " + extra);
            }
        }
        return bundledWallpapers;
    }

    public CropView getCropView() {
        return mCropView;
    }

    public SavedWallpaperImages getSavedImages() {
        return mSavedImages;
    }

    public void onLiveWallpaperPickerLaunch() {
        mLiveWallpaperInfoOnPickerLaunch = WallpaperManager.getInstance(this).getWallpaperInfo();
    }

    static class ZeroPaddingDrawable extends LevelListDrawable {
        public ZeroPaddingDrawable(Drawable d) {
            super();
            addLevel(0, 0, d);
            setLevel(0);
        }

        @Override
        public boolean getPadding(Rect padding) {
            padding.set(0, 0, 0, 0);
            return true;
        }
    }

    private static class BuiltInWallpapersAdapter extends BaseAdapter implements ListAdapter {
        private LayoutInflater mLayoutInflater;
        private ArrayList<ResourceWallpaperInfo> mWallpapers;

        BuiltInWallpapersAdapter(Activity activity, ArrayList<ResourceWallpaperInfo> wallpapers) {
            mLayoutInflater = activity.getLayoutInflater();
            mWallpapers = wallpapers;
        }

        public int getCount() {
            return mWallpapers.size();
        }

        public ResourceWallpaperInfo getItem(int position) {
            return mWallpapers.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            Drawable thumb = mWallpapers.get(position).mThumb;
            if (thumb == null) {
                Log.e(TAG, "Error decoding thumbnail for wallpaper #" + position);
            }
            return createImageTileView(mLayoutInflater, position, convertView, parent, thumb);
        }
    }

    public static View createImageTileView(LayoutInflater layoutInflater, int position,
            View convertView, ViewGroup parent, Drawable thumb) {
        View view;

        if (convertView == null) {
            view = layoutInflater.inflate(R.layout.wallpaper_picker_item, parent, false);
        } else {
            view = convertView;
        }

        setWallpaperItemPaddingToZero((FrameLayout) view);

        ImageView image = (ImageView) view.findViewById(R.id.wallpaper_image);

        if (thumb != null) {
            image.setImageDrawable(thumb);
            thumb.setDither(true);
        }

        return view;
    }

    /* SPRD: Fix bug 265306,set a new item to be selected. @{ */
    private void selectNewItem() {
        int count = mWallpapersView.getChildCount();
        for (int i = 0; i < count; i++) {
            WallpaperTileInfo wallpaperTileInfo = null;
            CheckableFrameLayout c =
                    (CheckableFrameLayout) mWallpapersView.getChildAt(i);
            Object object = c.getTag();
            if (object != null  && object instanceof WallpaperTileInfo) {
                wallpaperTileInfo = (WallpaperTileInfo) object;
            }
            if (wallpaperTileInfo != null && wallpaperTileInfo.isSelectable()) {
                if (mSelectedThumb != null) {
                    mSelectedThumb.setSelected(false);
                    mSelectedThumb = null;
                }
                mSelectedThumb = c;

                /* SPRD: bug282258 2014-02-22 wrong checked state with the thumb view @{ */
                mSelectedThumb.setSelected(true);
                /* SPRD: bug282258 2014-02-22 wrong checked state with the thumb view @} */

                wallpaperTileInfo.onClick(WallpaperPickerActivity.this);
                break;
            }
        }
    }
    /* @} */

    /* SPRD: Fix bug 264823 @{ */
    @SuppressWarnings("deprecation")
    @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
    private Point getDefaultDisplaySize(Point size) {
        Display d = getWindowManager().getDefaultDisplay();
        if (Build.VERSION.SDK_INT >= SDK_VERSION_HONEYCOMB_MR2) {
            d.getSize(size);
        } else {
            size.set(d.getWidth(), d.getHeight());
        }
        return size;
    }
    /* @} */

    /*
     * SPRD:fix bug 272920, set big picture crash: java.lang.OutOfMemoryError. @{
     */
     @Override
     protected void onDestroy() {
         Log.d(TAG, "WallpaperPickerActivity.onDestroy enter.");
         super.onDestroy();
         if(mlastPhoto != null){
             mlastPhoto.recycle();
         }
         Log.d(TAG, "WallpaperPickerActivity.onDestroy exit.");
     }
     /* @} */

    private void handlePickedImageResult(Uri uri) {
        if (uri == null) {
            Log.w(TAG, "handlePickedImageResult, result uri is null.");
            return;
        }
        /* SPRD: Fix bug 264823 @{ */
        Intent cropAndSetWallpaperIntent = null;
        boolean needCrop = false;
        String mimeType = this.getContentResolver().getType(uri);
        if (mimeType == null) {
            /* SPRD: bug332730 2014-07-14 use MediaFile to get mime type from file path. @{ */
            mimeType = SprdUtilities.getMimeTypeBaseOnFileExtension(uri.toString());
            /* SPRD: bug332730 2014-07-14 use MediaFile to get mime type from file path. @} */
        }
        if (mimeType != null) {
            needCrop = !(mimeType.equalsIgnoreCase("image/jpeg") || mimeType.equalsIgnoreCase("image/png"));
        }
        if (!needCrop) {
            addTemporaryWallpaperTile(uri);
        } else {
            int width = getWallpaperDesiredMinimumWidth();
            int height = getWallpaperDesiredMinimumHeight();
            Point size = getDefaultDisplaySize(new Point());
            float spotlightX = (float) size.x / width;
            float spotlightY = (float) size.y / height;
            cropAndSetWallpaperIntent = new Intent(CROP_ACTIVITY_ACTION);
            ComponentName name = new
                    ComponentName(GALLERY_PKG_NAME,CROP_ACTIVITY_CLASS);
            cropAndSetWallpaperIntent.setComponent(name);

            /* SPRD: Modify 20140116 of bug 269923,gallery crash when set wallpaper @{ */
            /* SPRD: bug274687 2014-01-24 can not set lockscreen wallpaper @{ */
            cropAndSetWallpaperIntent.setDataAndType(uri, "image/*")
                .addFlags(Intent.FLAG_ACTIVITY_FORWARD_RESULT)
                .putExtra(KEY_OUTPUT_X, width)
                .putExtra(KEY_OUTPUT_Y, height)
                .putExtra(KEY_ASPECT_X, width)
                .putExtra(KEY_ASPECT_Y, height)
                .putExtra(KEY_SPOTLIGHT_X, spotlightX)
                .putExtra(KEY_SPOTLIGHT_Y, spotlightY)
                .putExtra(KEY_SCALE, true)
                .putExtra(KEY_SCALE_UP_IF_NEEDED, true)
                .putExtra(KEY_SET_AS_WALLPAPER, true)
                .putExtra(WallpaperInfo.WALLPAPER_TARGET_ID, mWallPageTargetId)
                .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                            | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            /* SPRD: bug274687 2014-01-24 can not set lockscreen wallpaper @} */
            /* @} */

            startActivity(cropAndSetWallpaperIntent);
            finish();
        }
        /* @} */
    }

    private void initWallpaperListAsync() {
        // Make its background the last photo taken on external storage
        final Handler uihandler = new Handler();

        final ProgressBar progressBar = (ProgressBar) findViewById(R.id.loading);
        progressBar.setVisibility(View.VISIBLE);

        final LinearLayout masterWallpaperList = (LinearLayout) findViewById(R.id.master_wallpaper_list);
        final FrameLayout pickImageTile = (FrameLayout) getLayoutInflater()
                .inflate(R.layout.wallpaper_picker_image_picker_item,
                        masterWallpaperList, false);
        final LinearLayout liveWallpapersView = (LinearLayout) findViewById(R.id.live_wallpaper_list);
        final LinearLayout thirdPartyWallpapersView = (LinearLayout) findViewById(R.id.third_party_wallpaper_list);

        new Thread() {
            public void run() {
                ArrayList<ResourceWallpaperInfo> wallpapers = findBundledWallpapers();
                final BuiltInWallpapersAdapter builtInWallpapersAdapter = new BuiltInWallpapersAdapter(WallpaperPickerActivity.this, wallpapers);
                mSavedImages = new SavedWallpaperImages(WallpaperPickerActivity.this);
                mSavedImages.loadThumbnailsAndImageIdList();
                uihandler.post(new Runnable() {
                    public void run() {
                        /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @{ */
                        populateWallpapersFromAdapter(mWallpapersView, mSavedImages, true, false, true);
                        populateWallpapersFromAdapter(mWallpapersView, builtInWallpapersAdapter, false, false, true);
                        /* SPRD: bug314370 2014-05-20 buildin wallpapers's order is wrong when after onRestoreInstanceState. @} */
                    }
                });

                int wallPaperId = mWallPageTargetId;
                if (wallPaperId == WallpaperInfo.WALLPAPER_DEFAULT_TYPE) {
                    // Populate the live wallpapers
                    final LiveWallpaperListAdapter liveWallpaperListAdapter = new LiveWallpaperListAdapter(
                            WallpaperPickerActivity.this);
                    // Populate the third-party wallpaper pickers
                    final ThirdPartyWallpaperPickerListAdapter thirdPartyWallpaperPickerListAdapter = new ThirdPartyWallpaperPickerListAdapter(
                            WallpaperPickerActivity.this);

                    uihandler.post(new Runnable() {
                        public void run() {
                            liveWallpaperListAdapter.registerDataSetObserver(new DataSetObserver() {
                                public void onChanged() {
                                    liveWallpapersView.removeAllViews();
                                    populateWallpapersFromAdapter(liveWallpapersView, liveWallpaperListAdapter, false,
                                            false);
                                    initializeScrollForRtl();
                                    updateTileIndices();
                                }
                            });
                            populateWallpapersFromAdapter(thirdPartyWallpapersView, thirdPartyWallpaperPickerListAdapter, false,
                                    false);
                        }
                    });
                }

                uihandler.post(new Runnable() {
                    public void run() {
                        updateTileIndices();
                        selectNewItem();
                        // Update the scroll for RTL
                        initializeScrollForRtl();

                        // Init image picker tile
                        setWallpaperItemPaddingToZero(pickImageTile);
                        masterWallpaperList.addView(pickImageTile, 0);
                        PickImageInfo pickImageInfo = new PickImageInfo();
                        pickImageTile.setTag(pickImageInfo);
                        pickImageTile.setOnClickListener(mThumbnailOnClickListener);
                        pickImageInfo.setView(pickImageTile);

                        // Create smooth layout transitions for when items are deleted
                        initializeScrollForRtl();
                        final LayoutTransition transitioner = new LayoutTransition();
                        transitioner.setDuration(200);
                        transitioner.setStartDelay(LayoutTransition.CHANGE_DISAPPEARING, 0);
                        transitioner.setAnimator(LayoutTransition.DISAPPEARING, null);
                        mWallpapersView.setLayoutTransition(transitioner);
                        progressBar.setVisibility(View.INVISIBLE);
                    }
                });

                mlastPhoto = getThumbnailOfLastPhoto();

                uihandler.post(new Runnable() {
                    public void run() {
                        if (mlastPhoto != null) {
                            ImageView galleryThumbnailBg =
                                    (ImageView) pickImageTile.findViewById(R.id.wallpaper_image);
                            galleryThumbnailBg.setImageBitmap(mlastPhoto);
                            int colorOverlay = getResources().getColor(
                                    R.color.wallpaper_picker_translucent_gray);
                            galleryThumbnailBg.setColorFilter(colorOverlay,
                                    PorterDuff.Mode.SRC_ATOP);
                        }
                    }
                });
            }
        }.start();
    }

}
