// MainActivity.java - Red Themed Version
package com.android.support;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

public class MainActivity extends Activity {

    public static String libname = "libgspace1.so";
    public static String downloadurl = "https://darmodzdevxc.x10.mx/ProtectMe/libgspace1.so";
    public final String GameActivity = "com.tencent.tmgp.cod.CODMainActivity";

    public boolean hasLaunched = false;
    static Dialog neumorphicDialog;
    static TextView titleText, subtitleText, percentageText, statusText;
    static ProgressBar progressBar;
    static long serverLastModified;
    static int fileLength = 0;
    static int currentProgress = 0;

    static Context context;
    static Activity activity;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        context = this;
        activity = this;

        if (hasLaunched) return;
        hasLaunched = true;

        if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {
            Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + this.getPackageName()));
            this.startActivity(intent);
        }

        createNeumorphicDialog();
        String savepath = this.getFilesDir().getAbsolutePath() + "/" + libname;
        new FileDownloadTask().execute(downloadurl, savepath);
    }

    private void createNeumorphicDialog() {
        neumorphicDialog = new Dialog(this);
        neumorphicDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        neumorphicDialog.setCancelable(false);

        LinearLayout mainContainer = new LinearLayout(this);
        mainContainer.setOrientation(LinearLayout.VERTICAL);
        mainContainer.setGravity(Gravity.CENTER);
        mainContainer.setPadding(80, 80, 80, 80);

        GradientDrawable mainBackground = new GradientDrawable();
        mainBackground.setShape(GradientDrawable.RECTANGLE);
        mainBackground.setCornerRadius(32f);
        mainBackground.setColors(new int[]{
									 Color.parseColor("#FFCDD2"),
									 Color.parseColor("#FF8A80"),
									 Color.parseColor("#FFCDD2")
								 });
        mainBackground.setGradientType(GradientDrawable.RADIAL_GRADIENT);
        mainBackground.setGradientRadius(400f);
        mainBackground.setStroke(2, Color.parseColor("#E53935"));
        mainContainer.setBackground(mainBackground);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) mainContainer.setElevation(12f);

        // Icon
        LinearLayout iconContainer = new LinearLayout(this);
        iconContainer.setGravity(Gravity.CENTER);
        iconContainer.setPadding(0, 0, 0, 40);

        GradientDrawable iconBg = new GradientDrawable();
        iconBg.setShape(GradientDrawable.OVAL);
        iconBg.setSize(100, 100);
        iconBg.setColors(new int[]{Color.parseColor("#FF8A80"), Color.parseColor("#FF5252")});
        iconBg.setStroke(2, Color.parseColor("#E53935"));

        TextView iconText = new TextView(this);
        iconText.setText("⬇");
        iconText.setTextSize(28);
        iconText.setTextColor(Color.parseColor("#D32F2F"));
        iconText.setGravity(Gravity.CENTER);
        iconText.setBackground(iconBg);
        iconText.setWidth(100);
        iconText.setHeight(100);
        iconText.setShadowLayer(4, 0, 2, Color.parseColor("#20000000"));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) iconText.setElevation(4f);
        iconContainer.addView(iconText);
        mainContainer.addView(iconContainer);

        // Title
        titleText = new TextView(this);
        titleText.setText("Cluddy Library Update");
        titleText.setTextSize(26);
        titleText.setTextColor(Color.parseColor("#B71C1C"));
        titleText.setGravity(Gravity.CENTER);
        titleText.setPadding(0, 0, 0, 12);
        titleText.setShadowLayer(2, 0, 1, Color.parseColor("#30000000"));
        titleText.getPaint().setFakeBoldText(true);
        mainContainer.addView(titleText);

        // Subtitle
        subtitleText = new TextView(this);
        subtitleText.setText("Preparing download...");
        subtitleText.setTextSize(16);
        subtitleText.setTextColor(Color.parseColor("#D32F2F"));
        subtitleText.setGravity(Gravity.CENTER);
        subtitleText.setPadding(0, 0, 0, 56);
        mainContainer.addView(subtitleText);

        // Progress Container
        LinearLayout progressContainer = new LinearLayout(this);
        progressContainer.setOrientation(LinearLayout.VERTICAL);
        progressContainer.setPadding(20, 20, 20, 20);
        GradientDrawable progressBg = new GradientDrawable();
        progressBg.setShape(GradientDrawable.RECTANGLE);
        progressBg.setCornerRadius(20f);
        progressBg.setColor(Color.parseColor("#FFEBEE"));
        progressBg.setStroke(2, Color.parseColor("#E53935"));
        progressContainer.setBackground(progressBg);

        LinearLayout progressBarContainer = new LinearLayout(this);
        progressBarContainer.setOrientation(LinearLayout.HORIZONTAL);
        progressBarContainer.setPadding(0, 0, 0, 20);

        GradientDrawable progressTrackBg = new GradientDrawable();
        progressTrackBg.setShape(GradientDrawable.RECTANGLE);
        progressTrackBg.setCornerRadius(12f);
        progressTrackBg.setColor(Color.parseColor("#FFCDD2"));
        progressTrackBg.setStroke(1, Color.parseColor("#FF8A80"));

        progressBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progressBar.setMax(100);
        progressBar.setProgress(0);
        progressBar.setBackground(progressTrackBg);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            progressBar.setProgressTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#C62828")));
            progressBar.setProgressBackgroundTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#FFCDD2")));
        }
        progressBar.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 24));
        progressBarContainer.addView(progressBar);
        progressContainer.addView(progressBarContainer);

        // Texts below progress
        LinearLayout progressTextContainer = new LinearLayout(this);
        progressTextContainer.setOrientation(LinearLayout.HORIZONTAL);
        progressTextContainer.setPadding(8, 0, 8, 0);

        percentageText = new TextView(this);
        percentageText.setText("0%");
        percentageText.setTextSize(20);
        percentageText.setTextColor(Color.parseColor("#C62828"));
        percentageText.getPaint().setFakeBoldText(true);

        TextView fileInfoText = new TextView(this);
        fileInfoText.setText(libname);
        fileInfoText.setTextSize(13);
        fileInfoText.setTextColor(Color.parseColor("#8A1C1C"));
        fileInfoText.setGravity(Gravity.END);

        progressTextContainer.addView(percentageText);
        View spacer = new View(this);
        spacer.setLayoutParams(new LinearLayout.LayoutParams(0, 0, 1.0f));
        progressTextContainer.addView(spacer);
        progressTextContainer.addView(fileInfoText);
        progressContainer.addView(progressTextContainer);
        mainContainer.addView(progressContainer);

        // Status
        LinearLayout statusContainer = new LinearLayout(this);
        statusContainer.setOrientation(LinearLayout.HORIZONTAL);
        statusContainer.setGravity(Gravity.CENTER);
        statusContainer.setPadding(0, 32, 0, 0);

        TextView pulseDot = new TextView(this);
        pulseDot.setText("●");
        pulseDot.setTextSize(14);
        pulseDot.setTextColor(Color.parseColor("#E53935"));
        pulseDot.setPadding(0, 0, 16, 0);
        pulseDot.setShadowLayer(3, 0, 0, Color.parseColor("#40E53935"));
        statusContainer.addView(pulseDot);

        statusText = new TextView(this);
        statusText.setText("Initializing...");
        statusText.setTextSize(15);
        statusText.setTextColor(Color.parseColor("#B71C1C"));
        statusContainer.addView(statusText);
        mainContainer.addView(statusContainer);

        neumorphicDialog.setContentView(mainContainer);
        Window window = neumorphicDialog.getWindow();
        if (window != null) {
            window.setLayout(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            window.setBackgroundDrawableResource(android.R.color.transparent);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            }
            if (Build.VERSION.SDK_INT >= 31) {
                window.addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
            }
        }
    }

	private void updateProgress(final int progress, final String message) {
		if (neumorphicDialog != null && neumorphicDialog.isShowing()) {
			runOnUiThread(new Runnable() {
					@Override
					public void run() {
						if (progressBar != null) progressBar.setProgress(progress);
						if (percentageText != null) percentageText.setText(progress + "%");
						if (subtitleText != null) subtitleText.setText(message);
						if (statusText != null) {
							if (progress == 100) {
								statusText.setText("Finalizing installation");
								statusText.setTextColor(Color.parseColor("#D32F2F"));
							} else if (progress > 0) {
								statusText.setText("Downloading components " + progress + "%");
							} else {
								statusText.setText("Connecting to server...");
							}
						}
					}
				});
		}
	}
    private class FileDownloadTask extends AsyncTask<String, Integer, Boolean> {
        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            if (neumorphicDialog != null) neumorphicDialog.show();
        }

        @Override
        protected Boolean doInBackground(String... strings) {
            String fileURL = strings[0];
            String savePath = strings[1];
            try {
                URL url = new URL(fileURL);
                HttpURLConnection httpConn = (HttpURLConnection) url.openConnection();
                int responseCode = httpConn.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    serverLastModified = httpConn.getLastModified();
                    long storedLastModified = getLastModifiedTimeFromPrefs();
                    fileLength = httpConn.getContentLength();
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    int totalBytesRead = 0;
                    if (serverLastModified != storedLastModified) {
                        publishProgress(0);
                        File file = new File(savePath);
                        if (file.exists()) file.delete();
                        try (InputStream inputStream = httpConn.getInputStream();
						FileOutputStream outputStream = new FileOutputStream(savePath)) {
                            while ((bytesRead = inputStream.read(buffer)) != -1) {
                                outputStream.write(buffer, 0, bytesRead);
                                totalBytesRead += bytesRead;
                                currentProgress = (int) ((totalBytesRead * 100) / fileLength);
                                publishProgress(currentProgress);
                            }
                        }
                        return true;
                    } else {
                        publishProgress(100);
                        return false;
                    }
                }
                httpConn.disconnect();
            } catch (IOException e) {
                Log.e("Download", "Download error", e);
            }
            return false;
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            super.onProgressUpdate(values);
            int progress = values[0];
            String message = (progress == 0) ? "Preparing download..." : (progress == 100 ? "Installing library components" : "Downloading library files");
            updateProgress(progress, message);
        }

        @Override
        protected void onPostExecute(Boolean result) {
            if (neumorphicDialog != null && neumorphicDialog.isShowing()) neumorphicDialog.dismiss();
            File libFile = new File(MainActivity.this.getFilesDir().getAbsolutePath() + "/" + libname);
            if (result) {
                saveLastModifiedTime(serverLastModified);
                if (libFile.exists()) {
                    System.load(libFile.getAbsolutePath());
                    startGame();
                }
            } else if (libFile.exists()) {
                System.load(libFile.getAbsolutePath());
                startGame();
            }
        }

        private long getLastModifiedTimeFromPrefs() {
            return getSharedPreferences("library_prefs", MODE_PRIVATE).getLong("last_modified", 0);
        }

        private void saveLastModifiedTime(long lastModified) {
            getSharedPreferences("library_prefs", MODE_PRIVATE).edit().putLong("last_modified", lastModified).apply();
        }
    }

    private void startGame() {
        try {
            Intent intent = new Intent(MainActivity.this, Class.forName(GameActivity));
            startActivity(intent);
        } catch (ClassNotFoundException e) {
            Log.e("StartGame", "Game activity not found", e);
        }
    }
}
