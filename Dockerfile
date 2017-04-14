FROM alpine:3.4

MAINTAINER JulyRain <xx@xxoo.com>

ADD repositories /etc/apk/repositories 
ADD Shanghai /etc/localtime

COPY *.c /app/
COPY *.h /app/
COPY ev/ /app/ev/
COPY Makefile /app/

RUN apk add --no-cache --update make build-base && cd /app/ && make && rm -rf *.c *.h Makefile ev && apk del make build-base

WORKDIR /app

ENTRYPOINT ["/app/uot"]
