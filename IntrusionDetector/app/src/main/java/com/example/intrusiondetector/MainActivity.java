package com.example.intrusiondetector;

import android.os.Bundle;
import android.os.Handler;
import android.widget.Button;
import android.widget.TextView;
import retrofit2.*;
import retrofit2.converter.gson.GsonConverterFactory;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class MainActivity extends AppCompatActivity {

    TextView statusText, dist1Text, dist2Text;
    Button toggleButton, calibrateButton;

    ApiService apiService;

    Handler handler = new Handler();

    String BASE_URL = "http://10.239.190.73/"; //  CHANGE

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        statusText = findViewById(R.id.statusText);
        dist1Text = findViewById(R.id.dist1Text);
        dist2Text = findViewById(R.id.dist2Text);
        toggleButton = findViewById(R.id.toggleButton);
        calibrateButton = findViewById(R.id.calibrateButton);

        Retrofit retrofit = new Retrofit.Builder()
                .baseUrl(BASE_URL)
                .addConverterFactory(GsonConverterFactory.create())
                .build();

        apiService = retrofit.create(ApiService.class);

        // Start polling
        handler.post(updateRunnable);

        // Toggle button
        toggleButton.setOnClickListener(v -> {
            apiService.toggle().enqueue(new Callback<String>() {
                @Override
                public void onResponse(Call<String> call, Response<String> response) { }

                @Override
                public void onFailure(Call<String> call, Throwable t) { }
            });
        });

        // Calibrate button
        calibrateButton.setOnClickListener(v -> {
            apiService.calibrate().enqueue(new Callback<String>() {
                @Override
                public void onResponse(Call<String> call, Response<String> response) { }

                @Override
                public void onFailure(Call<String> call, Throwable t) { }
            });
        });
    }

    Runnable updateRunnable = new Runnable() {
        @Override
        public void run() {
            apiService.getStatus().enqueue(new Callback<StatusResponse>() {
                @Override
                public void onResponse(Call<StatusResponse> call, Response<StatusResponse> response) {
                    if (response.isSuccessful() && response.body() != null) {
                        StatusResponse data = response.body();

                        dist1Text.setText("Sensor1: " + data.dist1 + " cm");
                        dist2Text.setText("Sensor2: " + data.dist2 + " cm");

                        if (data.sensor1_Alarm || data.sensor2_Alarm) {
                            statusText.setText("⚠️ INTRUSION");
                        } else {
                            statusText.setText("SAFE");
                        }
                    }
                }

                @Override
                public void onFailure(Call<StatusResponse> call, Throwable t) {
                    statusText.setText("Connection Error");
                }
            });

            handler.postDelayed(this, 1000);
        }
    };
}