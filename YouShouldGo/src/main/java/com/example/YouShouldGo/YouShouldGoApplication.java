package com.example.YouShouldGo;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.annotation.Bean;
import org.springframework.web.client.RestClient;

@SpringBootApplication
public class YouShouldGoApplication {

    public static void main(String[] args) {
        SpringApplication.run(YouShouldGoApplication.class, args);
    }

    @Bean
    public RestClient restClient() {
        return RestClient.create();
    }
}