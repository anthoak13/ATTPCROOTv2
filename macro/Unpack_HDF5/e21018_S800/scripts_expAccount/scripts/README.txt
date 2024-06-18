
(Can find this procedure at https://docs.nscl.msu.edu/daq/newsite/ddas-1.1/singlecrate.html)
To create public and private key files, use ssh-keygen:

ssh-keygen -t rsa


Note that we will use an empty passphrase. Otherwise ssh will prompt us for the passphrase prior to using the keys, which leaves us no better off than before.

ssh-keygen will generate a pair of file: ~/.ssh/id_rsa and ~/.ssh/id_rsa.pub id_rsa.pub must be added to ~/.ssh/authorized_keys:

cat ~/.ssh/id_rsa.pub >>~/.ssh/authorized_keys

We're almost there. SSH wants to secure your account so that
Your private key; ~/.ssh/id_rsa canot be stolen and 
Nobody can implant additional keys into ~/.ssh/authorized_keys.

chmod g-w,o-w ~
chmod 0700 ~/.ssh
chmod 0600 ~/.ssh/id_rsa
chmod 0600 ~/.ssh/authorized_keys

Finally test this at the NSCL:

ssh fishtank

If you've followed this procedure properly, you should login to a fishtank system without being asked to give a password.
