const express = require('express');
const fs = require('fs');
const crypto = require('crypto');
const grpc = require('@grpc/grpc-js');
const { connect, signers, hash } = require('@hyperledger/fabric-gateway');

const peerEndpoint = 'peer0.org1.example.com:7051';
const certPem = fs.readFileSync('./keys/cert.pem');
const keyPem = fs.readFileSync('./keys/key.pem');
const tlsRoot = fs.readFileSync('./keys/ca.crt');

const client = new grpc.Client(
    peerEndpoint,
    grpc.credentials.createSsl(tlsRoot, undefined, undefined, {
        'grpc.ssl_target_name_override': 'peer0.org1.example.com',
        'grpc.default_authority': 'peer0.org1.example.com',
    })
);

const identity = { mspId: 'Org1MSP', credentials: certPem };
const signer = signers.newPrivateKeySigner(crypto.createPrivateKey(keyPem));

const gateway = connect({ identity, signer, hash: hash.sha256, client });
const network = gateway.getNetwork('sensordata');
const contract = network.getContract('sensorCC');

const app = express();
app.use(express.json());

app.get('/device/:uuid', async (req, res) => {
    try {
        const resultBytes = await contract.evaluateTransaction('QueryDevice', req.params.uuid);
        let jsonStr = resultBytes.toString();

        if (/^\d+,/.test(jsonStr)) {
            const byteArray = Uint8Array.from(
                jsonStr.split(',').map(n => parseInt(n, 10)));
            jsonStr = Buffer.from(byteArray).toString('utf8');
        }
        // console.log(jsonStr);
        const readings = JSON.parse(jsonStr);
        res.json(readings[readings.length - 1]);
    } catch (err) {
        console.error(err);
        res.status(500).json({ error: err.message });
    }
});

app.post('/reading', async (req, res) => {
    const r = req.body;
    const timestamp = new Date().getTime();
    r.timestamp = timestamp;
    console.log(r);

    const output = Object.fromEntries(
        Object.entries(r).map(([key, val]) => [
            key,
            (typeof val === 'string' && val.trim() !== '' && !isNaN(val))
                ? Number(val)
                : val
        ])
    );
    console.log(JSON.stringify(output));

    try {
        await contract.submitTransaction(
            'CreateReading',
            r.uuid,
            String(timestamp),
            JSON.stringify(output)
        );
        res.json({ status: 'committed' });
    } catch (err) {
        console.error(err);
        res.status(500).json({ error: err.message });
    }
});

const PORT = 3000;
app.listen(PORT, () => console.log(`REST API listening on :${PORT}`));

process.on('SIGINT', () => {
    gateway.close();
    client.close();
    process.exit();
});
