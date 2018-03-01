package com.android.castv2.test;

import android.app.Activity;
import android.os.Bundle;
import android.os.Looper;
import android.view.View;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.view.Surface;
import android.view.Display;
import android.content.Context;
import android.graphics.Point;
import android.media.MediaDrm;
import android.widget.TextView;
import android.media.MediaDrm.CryptoSession;
import android.media.MediaDrmException;
import android.media.NotProvisionedException;
import android.media.MediaCrypto;
import android.media.MediaCodec;
import android.media.MediaCryptoException;
import android.media.MediaCodec.CryptoException;
import android.media.MediaCodecList;
import android.media.MediaCodec.CryptoInfo;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.util.Log;
import android.util.TypedValue;
import android.util.AttributeSet;
import java.util.UUID;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.ListIterator;
import java.util.Iterator;
import java.util.HashMap;
import java.util.Random;
import java.nio.ByteBuffer;
import java.lang.Exception;
import java.lang.InterruptedException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.lang.Math;
import java.security.cert.CertificateFactory;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.PublicKey;
import java.security.Signature;
import java.security.SignatureException;
import java.security.InvalidKeyException;
import com.android.mediadrm.signer.MediaDrmSigner;
import java.io.InputStream;
import java.io.ByteArrayInputStream;
import dalvik.system.DexClassLoader;

public class CastSignAPITest extends Activity {
    private final String TAG = "CastSignAPITest";

    static final UUID kWidevineScheme = new UUID(0xEDEF8BA979D64ACEL, 0xA3C827DCD51D21EDL);

    private boolean mTestFailed;
    private TextView mTextView;
    private String mDisplayText;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        mTextView = new TextView(this);

        mDisplayText = new String("Cast Signing API Test\n\n");
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int width = size.x;
        int height = size.y;
        final int max_lines = 20;
        int textSize = Math.min(width, height) / max_lines;
        mTextView.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
        setContentView(mTextView);
        Log.i(TAG, "width=" + width + " height=" + height + " size=" + textSize);

        new Thread() {
            @Override
            public void run() {
                mTestFailed = false;

                testCastSign();

                if (mTestFailed) {
                    displayText("\nTEST FAILED!");
                } else {
                    displayText("\nTEST PASSED!");
                }
            }
        }.start();
    }

    // draw text on the surface view
    private void displayText(String text) {
        Log.i(TAG, text);
        mDisplayText += text + "\n";

        runOnUiThread(new Runnable() {
                public void run() {
                    mTextView.setText(mDisplayText);
                }
            });
    }

    private byte[] openSession(MediaDrm drm) {
        byte[] sessionId = null;
        boolean retryOpen;
        do {
            try {
                retryOpen = false;
                sessionId = drm.openSession();
            } catch (NotProvisionedException e) {
                Log.i(TAG, "Missing certificate, provisioning");
                ProvisionRequester provisionRequester = new ProvisionRequester();
                provisionRequester.doTransact(drm);
                retryOpen = true;
            }
        } while (retryOpen);
        return sessionId;
    }

    private void testCastSign() {
        final byte[] kMessage = hex2ba("ee07514066c23c770a665719abf051e0" +
                                       "f75a399578305eb2547ca67ecd2356ca" +
                                       "7bc0f5dc1854533eb98ee0fae1107ad2" +
                                       "a8707c671be10fcd4853990897b71378" +
                                       "70e39957a1536b39132e438ba681810d" +
                                       "ebc3f9fb38907a1066b09c63af9016a7" +
                                       "57425b29ff7fdf53859ebdc1659bdc49" +
                                       "f40a5cb5f597b0b8a836b424ad9c68a9");

        final byte[] kDigest = hex2ba(// digest info header
                                      "3021300906052b0e03021a05000414" +
                                      // sha1 of kMessage
                                      "d2662f893aaec72f3ca6decc2aa942f3949e8b21");

        MediaDrm drm;

        try {
            drm = new MediaDrm(kWidevineScheme);
        } catch (MediaDrmException e) {
            Log.e(TAG, "Failed to create MediaDrm: ", e);
            mTestFailed = true;
            return;
        }

        CertificateRequester certificateRequester = new CertificateRequester();
        MediaDrmSigner.Certificate signerCert = certificateRequester.doTransact(drm);
        if (signerCert == null) {
            displayText("Failed to get certificate from server!");
            mTestFailed = true;
        } else {
            byte[] sessionId = openSession(drm);

            byte[] signature = MediaDrmSigner.signRSA(drm, sessionId, "PKCS1-BlockType1",
                                                      signerCert.getWrappedPrivateKey(),
                                                      kDigest);

            try {
                byte[] certBuf = signerCert.getContent();
                InputStream certStream = new ByteArrayInputStream(certBuf);
                CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
                Certificate javaCert = certFactory.generateCertificate(certStream);
                PublicKey pubkey = javaCert.getPublicKey();
                Signature instance = Signature.getInstance("SHA1WithRSA");
                instance.initVerify(pubkey);
                instance.update(kMessage);
                if (instance.verify(signature)) {
                    Log.d(TAG, "Signatures match, test passed!");
                } else {
                    Log.e(TAG, "Signature did not verify, test failed!");
                    mTestFailed = true;
                }
            } catch (CertificateException e) {
                Log.e(TAG, "Failed to parse certificate", e);
                mTestFailed = true;
            } catch (NoSuchAlgorithmException e) {
                mTestFailed = true;
                Log.e(TAG, "Failed to get signature instance", e);
            } catch (InvalidKeyException e) {
                mTestFailed = true;
                Log.e(TAG, "Invalid public key", e);
            } catch (SignatureException e) {
                mTestFailed = true;
                Log.e(TAG, "Failed to update", e);
            }

            drm.closeSession(sessionId);
        }
    }

    private static byte[] hex2ba(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                                  + Character.digit(s.charAt(i+1), 16));
        }
        return data;
    }
}
