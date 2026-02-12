# HMM Regime Model

This section gives a practical explanation of the Hidden Markov Model (HMM) used by RegimeFlow.

## Symbols

Let:
- $K$ = number of regimes (states)
- $\mathbf{x}_t$ = feature vector at time $t$
- $z_t$ = hidden regime at time $t$
- $A_{ij}$ = transition probability from state $i$ to $j$
- $\pi_i$ = initial probability of state $i$
- $\mathcal{N}(\mu_i, \Sigma_i)$ = Gaussian emission for state $i$

## Conceptual Flow

```mermaid
flowchart LR
  A[Feature Vector] --> B[HMM Emission]
  B --> C[State Probabilities]
  C --> D[Transition Matrix]
  D --> E[Regime State]
```


## Formulas (LaTeX)

**Emission Probability**

$$
P(\mathbf{x}_t \mid z_t = i) = \mathcal{N}(\mathbf{x}_t; \mu_i, \Sigma_i)
$$

Interpretation: each regime explains observed features with its own Gaussian distribution.

**State Transition**

$$
P(z_t = j \mid z_{t-1} = i) = A_{ij}
$$

Interpretation: regime changes follow a fixed transition matrix.

**Filtered State Probability**

$$
P(z_t \mid \mathbf{x}_{1:t}) \propto P(\mathbf{x}_t \mid z_t) \sum_i A_{ij} P(z_{t-1}=i \mid \mathbf{x}_{1:t-1})
$$

Interpretation: current regime probability depends on prior state probabilities and the new observation.

## Practical Outputs

- **State probabilities** for each regime at each time step.
- **Most likely regime** based on maximum probability.
- **Transition metrics** when the dominant regime changes.

**Note:** The implementation includes optional Kalman smoothing for stability.
