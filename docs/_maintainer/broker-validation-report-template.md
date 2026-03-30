# Broker Validation Report Template

Use this template after every paper/demo broker validation run. Keep one filled report per broker, venue, and validation date.

## Header

- Date:
- Maintainer:
- Branch / commit:
- Broker:
- Venue:
- Account type:
- Region / network context:
- Build directory:
- Config path:
- Validation mode:

## Environment

- Symbols:
- Quantity:
- Limit price override:
- Runtime secret source:
- Build flags:
- Relevant env vars present:

## Startup Smoke

- Command:
- Result:
- Startup reached connected state:
- Heartbeat healthy:
- Audit log created:
- Secret leakage observed:
- Notes:

## Submit / Cancel / Reconcile

- Command:
- Result:
- Broker order id:
- Submit acknowledged:
- Visible in open-order reconciliation:
- Cancel accepted:
- Removed from reconciliation after cancel:
- Symbol / contract matched expected venue object:
- Notes:

## Fill / Reconcile

- Command:
- Result:
- Entry order id:
- Exit order id:
- Entry fill observed:
- Exit fill observed:
- Position reconciled correctly:
- Account equity / cash updated coherently:
- Residual position after exit:
- Notes:

## Observed Broker Constraints

- Min quantity / notional:
- TIF restrictions:
- Order-type restrictions:
- Market-data feed issues:
- Rate-limit observations:
- Region / policy issues:
- Other venue-specific behavior:

## Error Taxonomy Observed

- Auth:
- Network:
- Timeout:
- Rate-limit:
- Rejection:
- Schema / parse:
- Unknown / uncategorized:

## Final Assessment

- Startup validated:
- Lifecycle validated:
- Paper validated:
- Production validated:
- Blocking issues:
- Follow-up fixes required:

## Evidence

- Audit log path:
- Console log path:
- Screenshots / broker console evidence:
- Linked issue / PR:
