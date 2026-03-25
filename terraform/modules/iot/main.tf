resource "aws_iot_thing" "esp32_c6" {
  name = var.device_id
}

# Política que permite ao ESP32 conectar e publicar
resource "aws_iot_policy" "pubsub_policy" {
  name = "Policy_${var.device_id}"

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action   = ["iot:Connect", "iot:Publish", "iot:Receive", "iot:Subscribe"]
        Effect   = "Allow"
        Resource = "*"
      }
    ]
  })
}

# Regra de Tópico: Pega o JSON e insere no DynamoDB
resource "aws_iot_topic_rule" "to_dynamo" {
  name        = "SplitDataToDynamo"
  description = "Envia telemetria do ESP32 para o DynamoDB"
  enabled     = true
  sql         = "SELECT * FROM 'esp32/telemetria'"
  sql_version = "2016-03-23"

  dynamodbv2 {
    put_item {
      table_name = "TelemetriaMedidor"
    }
    role_arn = var.iam_role_arn # Role com permissão de escrita no Dynamo
  }
}