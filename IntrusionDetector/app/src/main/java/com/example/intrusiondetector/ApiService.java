package com.example.intrusiondetector;

import retrofit2.Call;
import retrofit2.http.GET;

public interface ApiService {

    @GET("/status")
    Call<StatusResponse> getStatus();

    @GET("/toggle")
    Call<String> toggle();

    @GET("/calibrate")
    Call<String> calibrate();
}