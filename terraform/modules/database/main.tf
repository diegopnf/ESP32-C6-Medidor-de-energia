resource "aws_dynamodb_table" "telemetria" {
  name           = "TelemetriaMedidor"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "device_id"   # Chave de partição (Partition Key)
  range_key      = "timestamp"   # Chave de classificação (Sort Key)

  attribute {
    name = "device_id"
    type = "S"
  }

  attribute {
    name = "timestamp"
    type = "N"
  }

  # --- OTIMIZAÇÃO DE CUSTOS (TTL) ---
  ttl {
    attribute_name = "expiracao" # O ESP32 ou uma Lambda deve enviar este campo (Epoch time)
    enabled        = true
  }

  tags = {
    Name        = "iot-database"
    Environment = "Dev"
  }
}

output "table_arn" {
  value = aws_dynamodb_table.telemetria.arn
}