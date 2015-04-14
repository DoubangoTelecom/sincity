# Uncomment next line to create "privkey.pem" and "SSL_CA.pem" files
openssl req -days 3650 -out SSL_CA.pem -new -x509
# Save privkey.pem and SSL_CA.pem

# General Public and private files
openssl genrsa -out SSL_Priv.pem 1024
openssl req -key SSL_Priv.pem -new -out ./cert.req
echo 00 > file.srl
openssl x509 -req -days 3650 -in cert.req -CA SSL_CA.pem -CAkey privkey.pem -CAserial file.srl -out SSL_Pub.pem

# To convert to DER
#openssl x509 -outform der -in SSL_CA.pem -out SSL_CA.der
#openssl x509 -outform der -in SSL_Pub.pem -out SSL_Pub.der

# To test client certs, run a server like this:
SSL routines:SSL23_GET_CLIENT_HELLO:unknown protocol
openssl s_server -accept 5061 -cert SSL_Pub.pem -CAfile SSL_CA.pem -key SSL_Priv.pem

# To convert to PKCS#12 (.pfx .p12)
openssl pkcs12 -export -out certificate.pfx -inkey SSL_Priv.pem -in SSL_Pub.pem -certfile SSL_CA.pem