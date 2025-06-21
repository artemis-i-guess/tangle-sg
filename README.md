# Lifecycle of a Transaction in the Microgrid Tangle Network

This document outlines the detailed, step-by-step lifecycle of a transaction in your DAG-based microgrid system, highlighting the roles of the sender, receiver, and other verifying nodes, along with the rationale for each step.

---

## 1. Sender: Create & Broadcast a Proposal

### 1.1 Assemble Transaction Body

```json
{
  "sender_id": S,
  "receiver_id": R,
  "units": U,
  "tariff_rate": T,
  "timestamp": t1
}
```

**Why?** Defines the proposed energy exchange terms.

### 1.2 Sign with Sender's Key

* Compute `h = H(body)`
* Create `sig_sender = Sign_SKs(h)`
* Append `sig_sender` to the body

**Why?** Binds the sender to these exact transaction terms.

### 1.3 Perform Minimal PoW (Difficulty d1, e.g. 2-4)

* Find `nonce1` such that `H(prefix || body || nonce1) < target(d1)`

**Why?** Prevents spam and enforces effort.

### 1.4 Select Parents

* **Parent A:** Sender's last approved transaction
* **Parent B:** A global tip selected via weighted algorithm

**Why?** Maintains per-user continuity and global DAG connectivity.

### 1.5 Broadcast Proposal (Tx1)

* Send: `(body, sig_sender, nonce1, parentA, parentB)`

**Why?** Announces the proposal to the network.

---

## 2. Any Node: Initial Verification & Gossip

### 2.1 Receive and Verify Tx1

* Validate PoW: `H(prefix || body || nonce1) < target(d1)`
* Validate `sig_sender`

**If valid:** Store and gossip
**If invalid:** Drop

**Why?** Ensures only well-formed proposals propagate.

---

## 3. Receiver: Detect & Inspect Proposal

### 3.1 Detect Proposal

* Monitor for transactions where `receiver_id == R`

**Why?** Identifies this node is the counterparty for this offer

### 3.2 Validate Proposal Content

* Confirm `units` and `tariff_rate`
* Verify `sig_sender` and PoW

**Why?** Prevents acceptance of incorrect or malicious data.

---

## 4. Receiver: Sign & Broadcast Approval

### 4.1 Generate Challenge Nonce

* Generate `n2 = SecureRandom()`
* Compute `sig_receiver = Sign_SKr(H(body) || n2)`

**Why?** Prevents replay attacks and binds approval to this proposal.

### 4.2 Perform PoW (Difficulty d2)

* Find `nonce3` such that `H(prefix || [Tx1_ID, n2, sig_receiver] || nonce3) < target(d2)`

**Why?** Adds cost to approvals and contributes to consensus weight.

### 4.3 Select Parents for Approval (Tx2)

* **Parent A:** Tx1 (the original proposal)
* **Parent B:** A global tip

### 4.4 Broadcast Approval (Tx2)

* Send: `{ref: Tx1_ID, n2, sig_receiver, nonce3, parentA, parentB}`

**Why?** Confirms the transaction publicly.

---

## 5. Any Node: Final Verification & Tip Selection

### 5.1 Validate Tx2

* Validate PoW
* Verify `sig_sender` from Tx1
* Recompute `H(body)`, then verify `sig_receiver(H(body) || n2)`

**If valid:** Mark Tx1 as approved
**If fail:** Reject Tx2 and donot propagate

### 5.2 Update Cumulative Weight

* Add weight of Tx2 to parents and propagate upstream

**Why?** Builds network consensus toward approved branches.

### 5.3 Tip Selection

* Prefer tips that are fully approved (Tx2 seen)
* Deprioritize or prune unapproved tips after timeout

**Why?** Keeps the DAG clean and consensus-focused.

---

## 6. Final Confirmation & Pruning

### 6.1 Confirmation Threshold

* Once cumulative weight > threshold: mark as confirmed

**Why?** Ensures finality

### 6.2 Prune Old Confirmed Transactions

* Archive or remove from memory for performance

**Why?** Saves memory and improves efficiency

> **Both 6.1 (Finality) and 6.2 (Pruning) are not implemented and marked as future scope. More discussion is needed as to how to best implement it without breaking the system.**

---

## Summary

- **Dual signatures (steps 1.2 & 4.1)** enforce both parties’ explicit consent.
- **Two-phase messages (Tx₁ & Tx₂)** respect content‑addressing immutability and give a full audit trail.
- **Minimal PoW at each phase** throttles spam and Sybil attack attempts twice over (on proposals and on approvals).
- **Hybrid parent‑selection** binds each bilateral exchange into the global consensus DAG, ensuring weight from every honest node flows into the same unified ledger.
- **Strict verification by every node (steps 2 & 5)** guarantees a permissionless, decentralized network—no single relay can fake or censor approvals.

This structured lifecycle ensures each energy transaction is fully auditable, mutually agreed upon, and cryptographically secure, while maintaining a fully decentralized architecture.
