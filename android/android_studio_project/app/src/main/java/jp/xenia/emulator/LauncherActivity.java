package jp.xenia.emulator;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;

public class LauncherActivity extends Activity {
    private static final int REQUEST_OPEN_GPU_TRACE_VIEWER = 0;
    private static final int REQUEST_OPEN_GAME = 1;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_launcher);
    }

    @Override
    protected void onActivityResult(
            final int requestCode, final int resultCode, final Intent data) {
        if (resultCode != RESULT_OK || data == null) {
            return;
        }
        final Uri uri = data.getData();
        if (uri == null) {
            return;
        }
        if (requestCode == REQUEST_OPEN_GPU_TRACE_VIEWER) {
            final Intent gpuTraceViewerIntent = new Intent(this, GpuTraceViewerActivity.class);
            final Bundle launchArguments = new Bundle();
            launchArguments.putString("target_trace_file", uri.toString());
            gpuTraceViewerIntent.putExtra(WindowedAppActivity.EXTRA_CVARS, launchArguments);
            startActivity(gpuTraceViewerIntent);
        } else if (requestCode == REQUEST_OPEN_GAME) {
            final Intent emulatorIntent = new Intent(this, EmulatorActivity.class);
            final Bundle launchArguments = new Bundle();
            launchArguments.putString("target", uri.toString());
            launchArguments.putString("apu", "android");
            launchArguments.putString("gpu", "vulkan");
            launchArguments.putString("hid", "android");
            emulatorIntent.putExtra(WindowedAppActivity.EXTRA_CVARS, launchArguments);
            startActivity(emulatorIntent);
        }
    }

    public void onLaunchEmulatorClick(final View view) {
        startActivity(new Intent(this, EmulatorActivity.class));
    }

    public void onLaunchOpenGameClick(final View view) {
        final Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, REQUEST_OPEN_GAME);
    }

    public void onLaunchGpuTraceViewerClick(final View view) {
        final Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        startActivityForResult(intent, REQUEST_OPEN_GPU_TRACE_VIEWER);
    }

    public void onLaunchWindowDemoClick(final View view) {
        startActivity(new Intent(this, WindowDemoActivity.class));
    }
}
