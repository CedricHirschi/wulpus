ARG CCS_VERSION=20.5.1.00012
FROM uoohyo/ccstudio-ide:${CCS_VERSION}

ENV CCS_VERSION=${CCS_VERSION}
ENV COMPONENTS=PF_MSP430
ENV PATH="/opt/ti/ccs/eclipse:${PATH}"

RUN ln -sf /bin/true /usr/local/bin/udevadm \
    && ln -sf /bin/true /sbin/start_udev \
    && ln -sf /bin/true /sbin/udevd \
    && ln -sf /bin/true /sbin/modprobe \
    && ln -sf /bin/true /sbin/insmod \
    && ln -sf /bin/true /sbin/rmmod \
    && mkdir -p /etc/udev/rules.d /run/udev /lib/modules /ccs_install \
    && cd "/opt/ccs-installer/CCS_${CCS_VERSION}_linux" \
    && chmod +x "ccs_setup_${CCS_VERSION}.run" \
    && "./ccs_setup_${CCS_VERSION}.run" --mode unattended --enable-components "${COMPONENTS}" --prefix /opt/ti \
    && test -x /opt/ti/ccs/eclipse/ccs-server-cli.sh \
    && rm -rf /opt/ccs-installer /ccs_install /tmp/*
