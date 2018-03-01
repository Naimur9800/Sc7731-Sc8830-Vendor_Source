package com.android.castv2.test;

import android.media.MediaDrm;
import com.android.mediadrm.signer.MediaDrmSigner;
import android.media.DeniedByServerException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.HttpClient;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.HttpResponse;
import org.apache.http.util.EntityUtils;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.util.Arrays;
import android.os.AsyncTask;
import android.util.Log;


public class CertificateRequester {
    private final String TAG = "CertificateRequester";

    public CertificateRequester() {
    }

    public MediaDrmSigner.Certificate doTransact(MediaDrm drm) {
        MediaDrmSigner.Certificate result = null;
        MediaDrmSigner.CertificateRequest certificateRequest =
            MediaDrmSigner.getCertificateRequest(drm, MediaDrmSigner.CERTIFICATE_TYPE_X509,
                                                 "cast.google.com");

        PostRequestTask postTask = new PostRequestTask(certificateRequest.getData());
        Log.i(TAG, "Requesting certificate from server '" + certificateRequest.getDefaultUrl() + "'");
        postTask.execute(certificateRequest.getDefaultUrl());

        // wait for post task to complete
        byte[] responseBody;
        long startTime = System.currentTimeMillis();
        do {
            responseBody = postTask.getResponseBody();
            if (responseBody == null) {
                sleep(100);
            } else {
                break;
            }
        } while (System.currentTimeMillis() - startTime < 5000);

        if (responseBody == null) {
            Log.e(TAG, "No response from certificate server!");
        } else {
            try {
                result = MediaDrmSigner.provideCertificateResponse(drm, responseBody);
            } catch (DeniedByServerException e) {
                Log.e(TAG, "Server denied certificate request");
            }
        }
        return result;
    }

    private class PostRequestTask extends AsyncTask<String, Void, Void> {
        private final String TAG = "PostRequestTask";

        private byte[] mCertificateRequest;
        private byte[] mResponseBody;

        public PostRequestTask(byte[] certificateRequest) {
            mCertificateRequest = certificateRequest;
        }

        protected Void doInBackground(String... urls) {
            mResponseBody = postRequest(urls[0], mCertificateRequest);
            if (mResponseBody != null) {
                Log.d(TAG, "response length=" + mResponseBody.length);
            }
            return null;
        }

        public byte[] getResponseBody() {
            return mResponseBody;
        }

        private byte[] postRequest(String url, byte[] certificateRequest) {
            HttpClient httpclient = new DefaultHttpClient();
            HttpPost httppost = new HttpPost(url + "&signedRequest=" + new String(certificateRequest));

            Log.d(TAG, "PostRequest:" + httppost.getRequestLine());

            try {
                // Add data
                httppost.setHeader("Accept", "*/*");
                httppost.setHeader("User-Agent", "Widevine CDM v1.0");
                httppost.setHeader("Content-Type", "application/json");

                // Execute HTTP Post Request
                HttpResponse response = httpclient.execute(httppost);

                byte[] responseBody;
                int responseCode = response.getStatusLine().getStatusCode();
                if (responseCode == 200) {
                    responseBody = EntityUtils.toByteArray(response.getEntity());
                } else {
                    Log.d(TAG, "Server returned HTTP error code " + responseCode);
                    return null;
                }
                return responseBody;

            } catch (ClientProtocolException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
            return null;
        }
    }

    private void sleep(int msec) {
        try {
            Thread.sleep(msec);
        } catch (InterruptedException e) {
        }
    }
}

