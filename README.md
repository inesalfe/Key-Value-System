# Key-Value System

This is the final project for the "Systems Programming" course. This course is a masters course in the degree of Electrical and Compuuter Engeneering in Instituto Superior Técnico - Universidade de Lisboa.

The goal of this project was to implement a key-value store composed by several components presented below:

<img width="799" alt="Screenshot 2023-03-01 at 16 39 33" src="https://user-images.githubusercontent.com/31959975/222204384-347c830a-dc9a-4b80-908d-c6d5ec972df8.png">

## Application

An application can be launched with the executable "KVS-lib". They can store one or more key-value pairs in a given group. The user can call any of the following functions:
* establish_connection - This function receives as arguments the strings containing the group name and corresponding secret, and tries to open the connection with the KVS-LocalServer. If successfull, all following operations on key-value pairs are done in the group to which the application is now connected.
* put_value - This function tries to assign the provided value to the provided key. If the provided key-pair does not exist in the server, it is created. If the key-value pair already exists in the server it is updated with the provided value.
* get_value - This function tries to retrieve the value associated to the provided key.
* delete_value - This function tries to delete the pair key-value associated to the provided key.
* register_callback - This function receives a key and tries to register a callback function such that, if the value associated to the provided key is changed, the user in this application should be informed
* close_connection - This function closes the connection to the currently connected group.

## Local Server

A local server can be created with the executable "KVS-LocalServer". A few options are available in each local server:
* Create Group - receives the identifier of a group (a string) and prints a secret (a string).
* Delete group – removes the group (by deleting the secret and removing all associated data.
* Show group info - prints the secret of the group and the number of key-value pairs.
* Show application status - lists all currently and past connected applications, namely the clients' PID and connection and closing times.

## Authentification Server

An authentification server can be created with the executable "KVS-AuthServer". This server is responsible for storing all the information about all groups and all secrets in all local servers.

