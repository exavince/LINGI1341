#!/bin/bash
rm -f received_file input_file

#fichier aleatoire:
dd if=/dev/urandom of=input_file bs=1 count=512 &> /dev/null

./src/receiver & receiver_pid=$!

cleanup()
{
    kill -9 $receiver_pid
    exit 0
}
trap cleanup SIGINT  # Kill les process en arrière plan en cas de ^-C

./src/sender -f <input_file

