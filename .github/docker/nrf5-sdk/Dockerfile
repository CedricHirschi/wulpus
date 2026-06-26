FROM ubuntu:24.04

ARG NRF5_SDK_ARCHIVE=nRF5_SDK_17.1.0_ddde560
ARG NRF5_SDK_URL=https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v17.x.x/nRF5_SDK_17.1.0_ddde560.zip

ENV NRF5_SDK_ROOT=/opt/nrf5-sdk

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      ca-certificates \
      curl \
      gcc-arm-none-eabi \
      git \
      libnewlib-arm-none-eabi \
      make \
      unzip \
    && curl -fsSL "${NRF5_SDK_URL}" -o /tmp/nrf5-sdk.zip \
    && unzip -q /tmp/nrf5-sdk.zip -d /opt \
    && mv "/opt/${NRF5_SDK_ARCHIVE}" "${NRF5_SDK_ROOT}" \
    && rm -rf /tmp/nrf5-sdk.zip /var/lib/apt/lists/*
