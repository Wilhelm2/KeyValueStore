FROM ubuntu:22.04
WORKDIR /appDB
RUN apt update && apt install -y python2 make gcc


COPY src src 
COPY database* .
COPY execution.py .
COPY Makefile .
CMD ["python2", "execution.py"]